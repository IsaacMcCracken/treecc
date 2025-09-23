#include "node.h"

U32 tree_hash_string(String s) {
    return tree_hash_dbj2((Byte*)s.str, s.len);
}

U64 tree_node_hash(TreeNode *node) {
    U32 input_hash = tree_hash_dbj2((Byte*)node->inputs, sizeof(TreeNode*)*node->inputlen);
    return ((((U64)node->kind)<<48) | ((U64)input_hash)) + (U64)node->vint;
}

B32 tree_node_equal(TreeNode *a, TreeNode *b) {
    if (a->inputlen != b->inputlen) return 0;
    return a->kind == b->kind &&
    memcmp((Byte*)a->inputs, (Byte*)b->inputs, sizeof(TreeNode*) * a->inputlen) == 0;
}

TreeNode *tree_symbol_table_lookup(TreeSymbolTableNode *table, String s) {
    U32 hash = tree_hash_string(s);
    U32 hashv = hash % table->cap;
    TreeSymbolNodeCell **slot = &table->cells[hashv];
    while (*slot) {
        if (string_cmp(s, (*slot)->name) == 0) {
            return (*slot)->node;
        }

        slot = &((*slot)->next);
    }

    return 0;
}

// B32 tree_symbol_table_insert(TreeSymbolTableNode *table, String s, TreeNode *node) {
//     U32 hash = tree_hash_string(s);
//     U32 hashv = hash % table->cap;
//     TreeSymbolNodeCell **slot = &table->cells[hashv];
//     while (*slot) {
//         if (string_cmp(s, (*slot)->name) == 0) {
//             return 0;
//         }

//         slot = &((*slot)->next);
//     }

//     TreeSymbolNodeCell *cell = arena_push(table->arena, TreeSymbolNodeCell);
//     cell->name = s;
//     cell->node = node;

//     *slot = cell;
//     return 1;
// }

void tree_map_insert(TreeNodeMap *map, TreeNode *node) {
    U64 hash = tree_node_hash(node);
    U64 hashv = hash % map->cap;

    TreeNodeMapCell **slot = &map->cells[hash % map->cap];

    while (*slot) {
        if (tree_node_equal((*slot)->node, node)) {
            // printf("node = %p\n", (*slot)->node);
            return;
        }
        slot = &(*slot)->next;
    }

    TreeNodeMapCell *cell = arena_push(map->arena, TreeNodeMapCell);
    *slot = cell;
}

TreeNodeMap tree_map_init(Arena *arena, U64 map_cap) {
    TreeNodeMapCell **cells = arena_push_array(arena, TreeNodeMapCell*, map_cap);
    TreeNodeMap map = (TreeNodeMap){
        .arena = arena,
        .cells = cells,
        .cap = map_cap,
    };

    return map;
}

TreeNode *tree_map_lookup(TreeNodeMap *map, TreeNode *node) {
    U64 hash = tree_node_hash(node);
    TreeNodeMapCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        printf("node = %p slot = %p\n", (*slot)->node, *slot);
        if (tree_node_equal((*slot)->node, node)) {
            return (*slot)->node;
        }
        slot = &(*slot)->next;
    }

    return NULL;
}

// TODO fix this
void tree_map_delete(TreeNodeMap *map, TreeNode *node) {
    U64 hash = tree_node_hash(node);
    TreeNodeMapCell **slot = &map->cells[hash % map->cap];
    TreeNodeMapCell *prev = NULL;
    while (*slot) {
        if (tree_node_equal((*slot)->node, node)) {
            TreeNodeMapCell *cell = *slot;
            TreeNodeMapCell *next = cell->next;
            if (prev) {
                prev->next = next;
            } else {
                *slot = cell->next;
            }
            cell->next = map->freelist;
            map->freelist = cell;
            return;
        }
        prev = *slot;
        slot = &(*slot)->next;
    }

}

void tree_node_alloc_inputs(TreeFunctionGraph *fn, TreeNode *node, U16 cap) {
    assert(node->inputs == NULL);
    node->inputs = arena_push_array(fn->arena, TreeNode*, cap);
    node->inputcap = cap;
}

void tree_node_alloc_users(TreeFunctionGraph *fn, TreeNode *node, U16 cap) {
    assert(node->users == NULL);
    node->users = arena_push_array(fn->arena, TreeUser, cap);
    node->usercap = cap;
}

void tree_node_append_user(TreeFunctionGraph *fn, TreeNode *node, TreeNode *user, U16 slot) {
    if (node->users == NULL) {
        tree_node_alloc_users(fn, node, 4);
    }

    if (node->userlen >= node->usercap) {
        U16 newcap = node->usercap * 2;
        TreeUser *new_users = arena_push_array(fn->arena, TreeUser, newcap);
        mem_cpy_array(new_users, node->users, TreeUser, node->userlen);
        node->users = new_users;
    }

    node->users[node->userlen] = (TreeUser){user, slot};
    node->userlen += 1;
}

void tree_node_set_input(TreeFunctionGraph *fn, TreeNode *node, TreeNode *input, U16 slot) {
    assert(slot < node->inputcap);
    node->inputlen = (node->inputlen > slot) ? node->inputlen : slot;
    node->inputs[slot] = input;
    if (input) {
        tree_node_append_user(fn, input, node, slot);
    }
}

TreeNode *tree_node_register(TreeFunctionGraph *fn, TreeNode *node) {
    TreeNode *canonical = tree_map_lookup(&fn->map, node);
    if (canonical) return canonical;

    canonical = arena_push(fn->arena, TreeNode);
    mem_cpy_item(canonical, node, TreeNode);

    tree_map_insert(&fn->map, canonical);
    return canonical;
}


TreeNode *tree_peepint(TreeFunctionGraph *fn, TreeNode *node) {
    TreeNode *lhs = node->inputs[0];
    TreeNode *rhs = node->inputs[1];
    // Constfold
    if (lhs->kind == TreeNodeKind_ConstInt && rhs->kind == TreeNodeKind_ConstInt) {
        S64 v = 0;
        switch (node->kind) {
            case TreeNodeKind_AddI: {
                v = lhs->vint + rhs->vint;
            } break;
            case TreeNodeKind_SubI: {
                v = lhs->vint - rhs->vint;
            } break;
            case TreeNodeKind_MulI: {
                v = lhs->vint * rhs->vint;
            } break;
            case TreeNodeKind_DivI: {
                v = lhs->vint / rhs->vint;
            } break;
        }
        // look in so see if we need to kill nodes
        return tree_create_const_int(fn, v);
    }

    if (node->kind == TreeNodeKind_AddI || node->kind == TreeNodeKind_MulI) {

        // if 2 * x -> x * 2  or 2 * (x * 4) -> (x * 4) * 2
        // swap sides because of communtivity
        if (lhs->kind == TreeNodeKind_ConstInt && rhs->kind != TreeNodeKind_ConstInt) {
            return tree_create_binary_expr(fn, node->kind, rhs, lhs);
        }

        // (x * 4) * 2 -> x * (4 * 2)
        // constant propagation
        if (lhs->kind == node->kind && lhs->inputs[1]->kind == TreeNodeKind_ConstInt && rhs->kind == TreeNodeKind_ConstInt) {
            TreeNode *new_rhs = tree_create_binary_expr(fn, node->kind, lhs->inputs[1], rhs);
            return tree_create_binary_expr(fn, node->kind, lhs->inputs[1], new_rhs);
        }

    }


    return node;
}

TreeNode *tree_peephole(TreeFunctionGraph *fn, TreeNode *node) {
    node = tree_node_register(fn, node);

    switch (node->kind) {
        case TreeNodeKind_AddI:
        case TreeNodeKind_SubI:
        case TreeNodeKind_MulI:
        case TreeNodeKind_DivI: {
            node = tree_peepint(fn, node);
        } break;
    }

    return node;
}


TreeNode *tree_create_const_int(TreeFunctionGraph *fn, S64 v) {
    TreeNode n = (TreeNode){
        .kind = TreeNodeKind_ConstInt,
        .vint = v,
    };
    // TODO set input to start
    return tree_peephole(fn, &n);
}

TreeNode *tree_create_binary_expr(
    TreeFunctionGraph *fn,
    TreeDataKind kind,
    TreeNode *lhs,
    TreeNode *rhs
) {
    TreeNode n = {
        .kind = kind,
    };
    tree_node_alloc_inputs(fn, &n, 2);
    tree_node_set_input(fn, &n, lhs, 0);
    tree_node_set_input(fn, &n, rhs, 1);

    return tree_peephole(fn, &n);
}

TreeNode *tree_create_proj(TreeFunctionGraph *fn, TreeNode *input, U16 v) {
    TreeNode n = {.kind = TreeNodeKind_Proj, .vint = v};
    // TODO: make a tree_meet_type(a, b) to data kind
    tree_node_alloc_inputs(fn, &n, 1);
    tree_node_set_input(fn, &n, input, 0);
    return tree_peephole(fn, &n);
}

TreeNode *tree_create_return(TreeFunctionGraph *fn, TreeNode *prev_ctrl, TreeNode *expr) {
    TreeNode n = {.kind = TreeNodeKind_Return};

    tree_node_alloc_inputs(fn, &n, 2);
    tree_node_set_input(fn, &n, prev_ctrl, 0);
    tree_node_set_input(fn, &n, expr, 1);

    return tree_peephole(fn, &n);
}
