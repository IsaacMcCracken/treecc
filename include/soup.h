
#include <core.h>

typedef U16 SoupDataKind;
enum {
  SoupDataKind_I64,
};

typedef U16 SoupNodeKind;
enum {
    SoupNodeKind_Invalid,

    // Control Flow
    SoupNodeKind_Start,
    SoupNodeKind_Stop,
    SoupNodeKind_Return,
    SoupNodeKind_If,
    SoupNodeKind_Region,

    // Data Operations
    SoupNodeKind_AddI,
    SoupNodeKind_SubI,
    SoupNodeKind_NegI,
    SoupNodeKind_MulI,
    SoupNodeKind_DivI,

    // Constants
    SoupNodeKind_ConstInt,
};


typedef struct SoupNode SoupNode;

typedef struct SoupUser SoupUser;
struct SoupUser {
  SoupNode *n;
  U16 slot;
};

struct SoupNode {
    SoupNodeKind kind;
    union {
        S64 vint;
    };
    SoupNode **inputs;
    SoupUser *users;
    U16 inputlen, userlen;
    U16 inputcap, usercap;
};

typedef struct SoupSymbolNodeCell SoupSymbolNodeCell;
struct SoupSymbolNodeCell {
    SoupSymbolNodeCell *next;
    String name;
    SoupNode *node;
};

typedef struct SoupSymbolTableNode SoupSymbolTableNode;
struct SoupSymbolTableNode {
    SoupNode node;
    SoupSymbolNodeCell **cells;
    U64 cap;
};

typedef struct SoupNodeMapCell SoupNodeMapCell;
struct SoupNodeMapCell {
    SoupNode *node;
    SoupNodeMapCell *next;
};

typedef struct SoupNodeMap SoupNodeMap;
struct SoupNodeMap {
    SoupNodeMapCell **cells;
    U64 cap;
    SoupNodeMapCell *freelist;
    Arena *arena;
};


typedef struct SoupFunction SoupFunction;
struct SoupFunction {
    Arena *arena;
    U64 deadspace;
    SoupNodeMap map;
    SoupNode *start;
};

// Builder Functions
SoupNode *soup_create_const_int(SoupFunction *fn, S64 v);
SoupNode *soup_create_binary_expr(SoupFunction *fn, SoupDataKind kind, SoupNode *lhs, SoupNode *rhs);

U32 soup_hash_dbj2(Byte *data, U64 len) {
    U32 hash = 5382;
    for (U32 i = 0; i < len; i++) {
        Byte c = data[i];
        hash = ((hash<<5) + hash) + c;
    }

    return hash;
}

U32 soup_hash_string(String s) {
    return soup_hash_dbj2((Byte*)s.str, s.len);
}

U64 soup_node_hash(SoupNode *node) {
    U32 input_hash = soup_hash_dbj2((Byte*)node->inputs, sizeof(SoupNode*)*node->inputlen);
    return ((((U64)node->kind)<<48) | ((U64)input_hash)) + (U64)node->vint;
}

B32 soup_node_equal(SoupNode *a, SoupNode *b) {
    if (a->inputlen != b->inputlen) return 0;
    return a->kind == b->kind &&
    memcmp((Byte*)a->inputs, (Byte*)b->inputs, sizeof(SoupNode*) * a->inputlen) == 0;
}

// void soup_symbol_table_insert(SoupSymbolTableNode *table, String s, SoupNode *node) {
//     U32 hash = soup_hash_string(s);
//     U32 hashv = hash % table->cap;
//     SoupSymbolNodeCell **slot = &table->cells[hashv];
//     while (*slot) {
//         if (string_cmp(s, (*slot)->name) == 0) {
//             (*slot)->node =node;
//             return;
//         }

//         slot = &((*slot)->next);
//     }

//     SoupSymbolNodeCell *cell = 
//     *slot = 
// }

void soup_map_insert(SoupNodeMap *map, SoupNode *node) {
    U64 hash = soup_node_hash(node);
    U64 hashv = hash % map->cap;
    SoupNodeMapCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (soup_node_equal((*slot)->node, node)) {
            return;
        }
        slot = &(*slot)->next;
    }

    SoupNodeMapCell *cell = arena_push(map->arena, SoupNodeMapCell);
    *slot = cell;
}

SoupNodeMap soup_map_init(U64 arena_cap, U64 map_cap) {
    Arena *arena = arena_init(arena_cap);
    SoupNodeMapCell **cells = arena_push_array(arena, SoupNodeMapCell*, map_cap);
    SoupNodeMap map = (SoupNodeMap){
        .arena = arena,
        .cells = cells,
        .cap = map_cap,
    };

    return map;
}

SoupNode *soup_map_lookup(SoupNodeMap *map, SoupNode *node) {
    U64 hash = soup_node_hash(node);
    SoupNodeMapCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (soup_node_equal((*slot)->node, node)) {
            return (*slot)->node;
        }
        slot = &(*slot)->next;
    }

    return NULL;
}

// TODO fix this
void soup_map_delete(SoupNodeMap *map, SoupNode *node) {
    U64 hash = soup_node_hash(node);
    SoupNodeMapCell **slot = &map->cells[hash % map->cap];
    SoupNodeMapCell *prev = NULL;
    while (*slot) {
        if (soup_node_equal((*slot)->node, node)) {
            SoupNodeMapCell *cell = *slot;
            SoupNodeMapCell *next = cell->next;
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

void soup_node_alloc_inputs(SoupFunction *fn, SoupNode *node, U16 cap) {
    assert(node->inputs == NULL);
    node->inputs = arena_push_array(fn->arena, SoupNode*, cap);
    node->inputcap = cap;
}

void soup_node_alloc_users(SoupFunction *fn, SoupNode *node, U16 cap) {
    assert(node->users == NULL);
    node->users = arena_push_array(fn->arena, SoupUser, cap);
    node->usercap = cap;
}

void soup_node_append_user(SoupFunction *fn, SoupNode *node, SoupNode *user, U16 slot) {
    if (node->users == NULL) {
        soup_node_alloc_users(fn, node, 4);
    }

    if (node->userlen >= node->usercap) {
        U16 newcap = node->usercap * 2;
        SoupUser *new_users = arena_push_array(fn->arena, SoupUser, newcap);
        mem_cpy_array(new_users, node->users, SoupUser, node->userlen);
        node->users = new_users;
    }

    node->users[node->userlen] = (SoupUser){user, slot};
    node->userlen += 1;
}

void soup_node_set_input(SoupFunction *fn, SoupNode *node, SoupNode *input, U16 slot) {
    assert(slot < node->inputcap);
    node->inputlen = (node->inputlen > slot) ? node->inputlen : slot;
    node->inputs[slot] = input;
    if (input) {
        soup_node_append_user(fn, input, node, slot);
    }
}

SoupNode *soup_node_register(SoupFunction *fn, SoupNode *node) {
    SoupNode *canonical = soup_map_lookup(&fn->map, node);
    if (canonical) return canonical;

    canonical = arena_push(fn->arena, SoupNode);
    mem_cpy_item(canonical, node, SoupNode);

    soup_map_insert(&fn->map, canonical);
    return canonical;
}


SoupNode *soup_peepint(SoupFunction *fn, SoupNode *node) {
    // Constfold
    if (node->inputs[0]->kind == SoupNodeKind_ConstInt && node->inputs[1]->kind == SoupNodeKind_ConstInt) {
        S64 v = 0;
        switch (node->kind) {
            case SoupNodeKind_AddI: {
                v = node->inputs[0]->vint + node->inputs[1]->vint;
            } break;
            case SoupNodeKind_SubI: {
                v = node->inputs[0]->vint - node->inputs[1]->vint;
            } break;
            case SoupNodeKind_MulI: {
                v = node->inputs[0]->vint * node->inputs[1]->vint;
            } break;
            case SoupNodeKind_DivI: {
                v = node->inputs[0]->vint / node->inputs[1]->vint;
            } break;
        }
        // look in so see if we need to kill nodes
        return soup_create_const_int(fn, v);
    }

    if (node->inputs[0]->kind == SoupNodeKind_ConstInt && node->inputs[1]->kind != SoupNodeKind_ConstInt) {
        return soup_create_binary_expr(fn, node->kind, node->inputs[1], node->inputs[0]);
    }

    return node;
}

SoupNode *soup_peephole(SoupFunction *fn, SoupNode *node) {
    node = soup_node_register(fn, node);

    switch (node->kind) {
        case SoupNodeKind_AddI:
        case SoupNodeKind_SubI:
        case SoupNodeKind_MulI:
        case SoupNodeKind_DivI: {
            node = soup_peepint(fn, node);
        } break;
    }

    return node;
}


SoupNode *soup_create_const_int(SoupFunction *fn, S64 v) {
    SoupNode n = (SoupNode){
        .kind = SoupNodeKind_ConstInt,
        .vint = v,
    };
    // TODO set input to start
    return soup_peephole(fn, &n);
}

SoupNode *soup_create_binary_expr(
    SoupFunction *fn,
    SoupDataKind kind,
    SoupNode *lhs,
    SoupNode *rhs
) {
    SoupNode n = {
        .kind = kind,
    };
    soup_node_alloc_inputs(fn, &n, 2);
    soup_node_set_input(fn, &n, lhs, 0);
    soup_node_set_input(fn, &n, rhs, 1);

    return soup_peephole(fn, &n);
}


SoupNode *soup_create_return(SoupFunction *fn, SoupNode *expr) {
    SoupNode n = {.kind = SoupNodeKind_Return};

    soup_node_alloc_inputs(fn, &n, 2);
    soup_node_set_input(fn, &n, expr, 1);

    return soup_peephole(fn, &n);
}
