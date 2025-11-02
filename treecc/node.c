#include "node.h"


void tree_node_print_expr_debug(TreeNode *expr) {
    switch (expr->kind) {
        case TreeNodeKind_Return: printf("return "); tree_node_print_expr_debug(expr->inputs[1]); break;
        case TreeNodeKind_ConstInt: printf("%ld", expr->vint); break;
        case TreeNodeKind_Proj: printf("arg%ld", expr->vint); break;
        case TreeNodeKind_AddI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" + ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_SubI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" - ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_MulI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" * ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_DivI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" / ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_GreaterThanI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" > ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_GreaterEqualI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" >= ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_LesserThanI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" < ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case TreeNodeKind_LesserEqualI: {
            putchar('(');
            tree_node_print_expr_debug(expr->inputs[0]);
            printf(" <= ");
            tree_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
    }
}


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
    mem_cmp((Byte*)a->inputs, (Byte*)b->inputs, sizeof(TreeNode*) * a->inputlen) == 0;
}

void tree_map_insert(TreeNodeMap *map, TreeNode *node) {
    U64 hash = tree_node_hash(node);
    U64 hashv = hash % map->cap;

    TreeNodeMapCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (tree_node_equal((*slot)->node, node)) {
            return;
        }
        slot = &(*slot)->next;
    }

    TreeNodeMapCell *cell = arena_push(map->arena, TreeNodeMapCell);
    cell->node = node;
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
        if (tree_node_equal((*slot)->node, node)) {
            return (*slot)->node;
        }
        slot = &(*slot)->next;
    }

    return NULL;
}

// TODO fix this
void tree_map_remove(TreeNodeMap *map, TreeNode *node) {
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

void tree_node_append_input(TreeFunctionGraph *fn, TreeNode *node, TreeNode *input) {
    if (node->inputs == 0) {
        tree_node_alloc_inputs(fn, node, 4);
    }

    if (node->userlen >= node->usercap) {
        U16 newcap = node->usercap * 2;
        TreeNode **new_inputs = arena_push_array(fn->arena, TreeNode*, newcap);
        mem_cpy_array(new_inputs, node->inputs, TreeUser, node->inputlen);
        node->inputs = new_inputs;
    }

    node->inputs[node->inputlen] = input;
    node->inputlen += 1;
}

void tree_node_remove_user(TreeNode *node, TreeNode *user) {
    // unordered remove
    U16 slot;
    for (slot = 0; slot < node->userlen; slot++) {
        if (node->users[slot].n == node) break;
    }

    if (slot < node->userlen - 1) {
        mem_cpy_item(&node->users[slot], &node->users[node->userlen - 1], TreeUser);
    }

    node->userlen -= 1;
}

void tree_node_set_input(TreeFunctionGraph *fn, TreeNode *node, TreeNode *input, U16 slot) {
    assert(slot < node->inputcap);
    node->inputlen = (node->inputlen > slot) ? node->inputlen : slot + 1; // this bad boy cause a big bug love those off by one errors
    node->inputs[slot] = input;
    if (input) {
        tree_node_append_user(fn, input, node, slot);
    }
}

TreeNode *tree_node_register(TreeFunctionGraph *fn, TreeNode *node) {
    TreeNode *canonical = tree_map_lookup(&fn->map, node);
    if (canonical) {
        return canonical;
    }

    canonical = arena_push(fn->arena, TreeNode);
    mem_cpy_item(canonical, node, TreeNode);

    tree_map_insert(&fn->map, canonical);
    return canonical;
}


TreeNode *tree_peepint(TreeFunctionGraph *fn, TreeNode *node) {
    // constfold urnary expression
    if (node->kind == TreeNodeKind_NegateI && (node->inputs[0]->kind == TreeNodeKind_ConstInt)) {
        return tree_create_const_int(fn, -node->inputs[0]->vint);
    } else  if (node->kind == TreeNodeKind_Not && node->inputs[0]->kind == TreeNodeKind_ConstInt) {
        return tree_create_const_int(fn, !node->inputs[0]->vint);
    }

    TreeNode *lhs = node->inputs[0];
    TreeNode *rhs = node->inputs[1];
    // Constfold binary expression
    if (lhs->kind == TreeNodeKind_ConstInt && rhs->kind == TreeNodeKind_ConstInt) {
        S64 v = 0;
        switch (node->kind) {
            case TreeNodeKind_EqualI: {
                v = lhs->vint == rhs->vint;
            } break;
            case TreeNodeKind_NotEqualI: {
                v = lhs->vint != rhs->vint;
            } break;
            case TreeNodeKind_GreaterEqualI: {
                v = lhs->vint >= rhs->vint;
            } break;
            case TreeNodeKind_GreaterThanI: {
                v = lhs->vint > rhs->vint;
            } break;
            case TreeNodeKind_LesserEqualI: {
                v = lhs->vint <= rhs->vint;
            } break;
            case TreeNodeKind_LesserThanI: {
                v = lhs->vint < rhs->vint;
            } break;
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
        if (lhs->kind == node->kind && lhs->inputs[1]->kind == TreeNodeKind_ConstInt) {
            TreeNode *new_rhs = tree_create_binary_expr(fn, node->kind, lhs->inputs[1], rhs);
            TreeNode *new_expr = tree_create_binary_expr(fn, node->kind, lhs->inputs[0], new_rhs);
            return new_expr;
        }

    }


    if (node->kind == TreeNodeKind_AddI && rhs == lhs) {
        if (lhs == rhs) {
            TreeNode *two = tree_create_const_int(fn, 2);
            return tree_create_binary_expr(fn, TreeNodeKind_MulI, lhs, two);
        }

        if (rhs->kind == TreeNodeKind_ConstInt && rhs->vint == 0) return lhs;
    }

    if (node->kind == TreeNodeKind_SubI && lhs == rhs) {
        TreeNode *zero = tree_create_const_int(fn, 0);
        return zero;
    }

    if (node->kind == TreeNodeKind_MulI && rhs->kind == TreeNodeKind_ConstInt && rhs->vint == 1) {
        return lhs;
    }

    if (node->kind == TreeNodeKind_DivI) {
        if (lhs == rhs) return tree_create_const_int(fn, 1);
        if (rhs->kind == TreeNodeKind_ConstInt && rhs->vint == 1) return lhs;
    }


    return node;
}

void tree_kill_node(TreeFunctionGraph *fn, TreeNode *node) {
    // garbage collection info
    fn->deadspace += sizeof(TreeNode);
    fn->deadspace += sizeof(TreeUser) * node->usercap;
    fn->deadspace += sizeof(TreeNode*) * node->inputcap;
    // TODO if there is extra stuff allocated for special nodes add to deadspace

    // todo copy graph is there is too much deadspace

    // remove from hash map (gvn)
    tree_map_remove(&fn->map, node);

    // remove user from inputs
    for (U16 i = 0; i < node->inputlen; i++) {
        tree_node_remove_user(node->inputs[i], node);
        node->inputs[i] = 0;
    }

    node->inputlen = 0;
    node->kind = TreeNodeKind_Invalid;
}

void tree_dead_code_elim(TreeFunctionGraph *fn, TreeNode *node) {
    if (node->userlen == 0) {
        tree_kill_node(fn, node);
    }
}

TreeNode *tree_peephole(TreeFunctionGraph *fn, TreeNode *node) {
    node = tree_node_register(fn, node);

    TreeNode *new = 0;
    switch (node->kind) {
        case TreeNodeKind_Not:
        case TreeNodeKind_EqualI:
        case TreeNodeKind_NotEqualI:
        case TreeNodeKind_GreaterThanI:
        case TreeNodeKind_GreaterEqualI:
        case TreeNodeKind_LesserThanI:
        case TreeNodeKind_LesserEqualI:
        case TreeNodeKind_NegateI:
        case TreeNodeKind_AddI:
        case TreeNodeKind_SubI:
        case TreeNodeKind_MulI:
        case TreeNodeKind_DivI: {
            new = tree_peepint(fn, node);
        } break;
    }

    if (new) {
        tree_dead_code_elim(fn, node);
    } else {
        new = node;
    }

    return new;
}



TreeNode *tree_create_const_int(TreeFunctionGraph *fn, S64 v) {
    TreeNode n = (TreeNode){
        .kind = TreeNodeKind_ConstInt,
        .vint = v,
    };
    // TODO set input to start
    return tree_peephole(fn, &n);
}

TreeNode *tree_create_urnary_expr(TreeFunctionGraph *fn, TreeNodeKind kind, TreeNode *input) {
    TreeNode n = (TreeNode){
        .kind = kind,
    };

    tree_node_alloc_inputs(fn, &n, 1);
    tree_node_set_input(fn, &n, input, 0);

    return tree_peephole(fn, &n);
}

TreeNode *tree_create_binary_expr(
    TreeFunctionGraph *fn,
    TreeDataType kind,
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

    TreeNode *final = tree_peephole(fn, &n);

    // add return to the stop node
    tree_node_append_input(fn, fn->stop, final);

    return final;
}

TreeNode *tree_create_phi2(TreeFunctionGraph *fn, TreeNode *region, TreeNode *a, TreeNode *b) {
    TreeNode n = {.kind = TreeNodeKind_Phi};

    tree_node_alloc_inputs(fn, &n, 3);
    tree_node_set_input(fn, &n, region, 0);
    tree_node_set_input(fn, &n, region, 1);
    tree_node_set_input(fn, &n, region, 2);

    return tree_peephole(fn, &n);
}

TreeNode *tree_create_if(TreeFunctionGraph *fn, TreeNode *prev_ctrl) {
    TreeNode n = {.kind = TreeNodeKind_If};

    tree_node_alloc_inputs(fn, &n, 1);
    tree_node_set_input(fn, &n, prev_ctrl, 0);

    tree_node_alloc_users(fn, &n, 2);

    return tree_peephole(fn, &n);
}

TreeNode *tree_create_region_for_if(TreeFunctionGraph *fn, TreeNode *t, TreeNode *f, U16 output_reserves) {
    TreeNode n = {.kind = TreeNodeKind_Region};

    tree_node_alloc_inputs(fn, &n, 2);
    tree_node_set_input(fn, &n, t, 0);
    tree_node_set_input(fn, &n, f, 1);

    if (output_reserves) tree_node_alloc_users(fn, &n, output_reserves);

    return tree_peephole(fn, &n);
}
