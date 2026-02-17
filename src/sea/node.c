#include "sea_internal.h"

void sea_node_print_expr_debug(SeaNode *expr) {
    switch (expr->kind) {
        case SeaNodeKind_Return: printf("return "); sea_node_print_expr_debug(expr->inputs[1]); break;
        case SeaNodeKind_ConstInt: printf("%ld", expr->vint); break;
        case SeaNodeKind_Proj: printf("arg%ld", expr->vint); break;
        case SeaNodeKind_AddI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" + ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_SubI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" - ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_MulI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" * ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_DivI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" / ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_GreaterThanI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" > ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_GreaterEqualI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" >= ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_LesserThanI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" < ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
        case SeaNodeKind_LesserEqualI: {
            putchar('(');
            sea_node_print_expr_debug(expr->inputs[0]);
            printf(" <= ");
            sea_node_print_expr_debug(expr->inputs[1]);
            putchar(')');
        } break;
    }
}




U64 sea_node_hash(SeaNode *node) {
    String8 data = (String8) {
        .str = (U8*)node->inputs,
        .size = sizeof(SeaNode*)*node->inputlen,
    };
    U64 input_hash = u64_hash_from_str8(data);
    return ((((U64)node->kind)<<48) | ((U64)input_hash)) + (U64)node->vint;
}

B32 sea_node_equal(SeaNode *a, SeaNode *b) {
    if (a->inputlen != b->inputlen) return 0;

    return a->kind == b->kind &&
    MemoryMatch((U8*)a->inputs, (U8*)b->inputs, sizeof(SeaNode*) * a->inputlen);
}

void sea_map_insert(SeaNodeMap *map, SeaNode *node) {
    U64 hash = sea_node_hash(node);
    U64 hashv = hash % map->cap;

    SeaNodeMapCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->node, node)) {
            return;
        }
        slot = &(*slot)->next;
    }

    SeaNodeMapCell *cell = push_item(map->arena, SeaNodeMapCell);
    cell->node = node;
    *slot = cell;
}

SeaNodeMap sea_map_init(Arena *arena, U64 map_cap) {
    SeaNodeMapCell **cells = push_array(arena, SeaNodeMapCell*, map_cap);
    SeaNodeMap map = (SeaNodeMap){
        .arena = arena,
        .cells = cells,
        .cap = map_cap,
    };

    return map;
}

SeaNode *sea_map_lookup(SeaNodeMap *map, SeaNode *node) {
    U64 hash = sea_node_hash(node);
    SeaNodeMapCell **slot = &map->cells[hash % map->cap];

    while (*slot) {
        if (sea_node_equal((*slot)->node, node)) {
            return (*slot)->node;
        }
        slot = &(*slot)->next;
    }

    return NULL;
}

// TODO fix this
void sea_map_remove(SeaNodeMap *map, SeaNode *node) {
    U64 hash = sea_node_hash(node);
    SeaNodeMapCell **slot = &map->cells[hash % map->cap];
    SeaNodeMapCell *prev = NULL;
    while (*slot) {
        if (sea_node_equal((*slot)->node, node)) {
            SeaNodeMapCell *cell = *slot;
            SeaNodeMapCell *next = cell->next;
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


void sea_node_alloc_inputs(SeaFunctionGraph *fn, SeaNode *node, U16 cap) {
    assert(node->inputs == NULL);
    node->inputs = push_array(fn->arena, SeaNode*, cap);
    node->inputcap = cap;
}

void sea_node_alloc_users(SeaFunctionGraph *fn, SeaNode *node, U16 cap) {
    assert(node->users == NULL);
    node->users = push_array(fn->arena, SeaUser, cap);
    node->usercap = cap;
}

void sea_node_append_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot) {
    if (node->users == NULL) {
        sea_node_alloc_users(fn, node, 4);
    }

    if (node->userlen >= node->usercap) {
        U16 newcap = node->usercap * 2;
        SeaUser *new_users = push_array(fn->arena, SeaUser, newcap);
        MemoryCopyTyped(new_users, node->users, node->userlen);
        node->users = new_users;
    }

    node->users[node->userlen] = (SeaUser){user, slot};
    node->userlen += 1;
}

U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input) {
    if (node->inputs == 0) {
        sea_node_alloc_inputs(fn, node, 4);
    }

    if (node->inputlen >= node->inputcap) {
        U16 newcap = node->inputcap * 2;
        SeaNode **new_inputs = push_array(fn->arena, SeaNode*, newcap);
        MemoryCopyTyped(new_inputs, node->inputs,node->inputlen);
        node->inputs = new_inputs;
    }

    U16 slot = node->inputlen;
    node->inputs[node->inputlen] = input;
    node->inputlen += 1;

    if (input) {
        sea_node_append_user(fn, input, node, slot);
    }

    return slot;
}

void sea_node_remove_user_slot(SeaNode *node, U16 slot) {
    assert(slot < node->userlen);

    if (slot < node->userlen - 1) {
        MemoryCopy(&node->users[slot], &node->users[node->userlen - 1], sizeof(SeaUser));
    }

    node->userlen -= 1;
}

void sea_node_remove_user(SeaNode *node, SeaNode *user) {
    // unordered remove
    // TODO is the slot in user the pint in the
    U16 slot;
    for (slot = 0; slot < node->userlen; slot++) {
        if (node->users[slot].n == user) {
            sea_node_remove_user_slot(node, slot);
            return;
        }
    }

    fprintf(stderr, "User not in node.\n");
    Trap();
}

void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot) {
    assert(slot < node->inputcap);

    if (node->inputs[slot]) {
        sea_node_remove_user(node->inputs[slot], node);
    }

    node->inputlen = (node->inputlen > slot) ? node->inputlen : slot + 1; // this bad boy cause a big bug love those off by one errors
    node->inputs[slot] = input;
    if (input) {
        sea_node_append_user(fn, input, node, slot);
    }
}

SeaNode *sea_node_register(SeaFunctionGraph *fn, SeaNode *node) {
    SeaNode *canonical = sea_map_lookup(&fn->map, node);
    if (canonical) {
        return canonical;
    }

    canonical = push_item(fn->arena, SeaNode);
    MemoryCopyStruct(canonical, node);

    sea_map_insert(&fn->map, canonical);
    return canonical;
}


SeaNode *sea_idealize_int(SeaFunctionGraph *fn, SeaNode *node) {
    // constfold urnary expression
    if (node->kind == SeaNodeKind_NegateI && (node->inputs[0]->kind == SeaNodeKind_ConstInt)) {
        return sea_create_const_int(fn, -node->inputs[0]->vint);
    } else  if (node->kind == SeaNodeKind_Not && node->inputs[0]->kind == SeaNodeKind_ConstInt) {
        return sea_create_const_int(fn, !node->inputs[0]->vint);
    }

    SeaNode *lhs = node->inputs[0];
    SeaNode *rhs = node->inputs[1];
    // Constfold binary expression
    if (lhs->kind == SeaNodeKind_ConstInt && rhs->kind == SeaNodeKind_ConstInt) {
        S64 v = 0;
        switch (node->kind) {
            case SeaNodeKind_EqualI: {
                v = lhs->vint == rhs->vint;
            } break;
            case SeaNodeKind_NotEqualI: {
                v = lhs->vint != rhs->vint;
            } break;
            case SeaNodeKind_GreaterEqualI: {
                v = lhs->vint >= rhs->vint;
            } break;
            case SeaNodeKind_GreaterThanI: {
                v = lhs->vint > rhs->vint;
            } break;
            case SeaNodeKind_LesserEqualI: {
                v = lhs->vint <= rhs->vint;
            } break;
            case SeaNodeKind_LesserThanI: {
                v = lhs->vint < rhs->vint;
            } break;
            case SeaNodeKind_AddI: {
                v = lhs->vint + rhs->vint;
            } break;
            case SeaNodeKind_SubI: {
                v = lhs->vint - rhs->vint;
            } break;
            case SeaNodeKind_MulI: {
                v = lhs->vint * rhs->vint;
            } break;
            case SeaNodeKind_DivI: {
                v = lhs->vint / rhs->vint;
            } break;
        }
        // look in so see if we need to kill nodes
        return sea_create_const_int(fn, v);
    }

    if (node->kind == SeaNodeKind_AddI || node->kind == SeaNodeKind_MulI) {

        // if 2 * x -> x * 2  or 2 * (x * 4) -> (x * 4) * 2
        // swap sides because of communtivity
        if (lhs->kind == SeaNodeKind_ConstInt && rhs->kind != SeaNodeKind_ConstInt) {
            return sea_create_bin_op(fn, node->kind, rhs, lhs);
        }


        // (x * 4) * 2 -> x * (4 * 2)
        // constant propagation
        if (lhs->kind == node->kind && lhs->inputs[1]->kind == SeaNodeKind_ConstInt) {
            SeaNode *new_rhs = sea_create_bin_op(fn, node->kind, lhs->inputs[1], rhs);
            SeaNode *new_expr = sea_create_bin_op(fn, node->kind, lhs->inputs[0], new_rhs);
            return new_expr;
        }

    }


    if (node->kind == SeaNodeKind_AddI && rhs == lhs) {
        if (lhs == rhs) {
            SeaNode *two = sea_create_const_int(fn, 2);
            return sea_create_bin_op(fn, SeaNodeKind_MulI, lhs, two);
        }

        if (rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 0) return lhs;
    }

    if (node->kind == SeaNodeKind_SubI && lhs == rhs) {
        SeaNode *zero = sea_create_const_int(fn, 0);
        return zero;
    }

    if (node->kind == SeaNodeKind_MulI && rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 1) {
        return lhs;
    }

    if (node->kind == SeaNodeKind_DivI) {
        if (lhs == rhs) return sea_create_const_int(fn, 1);
        if (rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 1) return lhs;
    }

    if (sea_type_is_const_int(node->type)) {
        return sea_create_const_int(fn, sea_type_const_int_val(node->type));
    }

    return node;
}


void sea_node_kill(SeaFunctionGraph *fn, SeaNode *node) {
    Assert(node->userlen == 0);
    // garbage collection info
    fn->deadspace += sizeof(SeaNode);
    fn->deadspace += sizeof(SeaUser) * node->usercap;
    fn->deadspace += sizeof(SeaNode*) * node->inputcap;
    // TODO if there is extra stuff allocated for special nodes add to deadspace

    // todo copy graph is there is too much deadspace

    // remove from hash map (gvn)
    sea_map_remove(&fn->map, node);

    // remove user from inputs
    for EachIndex(i, node->inputlen) {
        sea_node_remove_user(node->inputs[i], node);
        node->inputs[i] = 0;
    }

    node->inputlen = 0;
    node->kind = SeaNodeKind_Invalid;
}

void sea_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new) {
    Assert(old != new);
    for EachIndex(i, old->userlen) {
        SeaNode *user = old->users[i].n;
        U16 slot = old->users[i].slot;
        sea_node_set_input(fn, user, new, slot);
        printf("User %d users left %d\n", (int)i, (int)old->userlen);

    }
    sea_node_kill(fn, old);
}

SeaNode *sea_dead_code_elim(SeaFunctionGraph *fn, SeaNode *this, SeaNode *m) {
    if (this != m && this->userlen == 0) {
        sea_node_kill(fn, this);
    }

    return m;
}

SeaNode *sea_idealize_phi(SeaFunctionGraph *fn, SeaNode *node) {
    U16 slot;
    for (slot = 2; slot < node->inputlen; slot++) {
        if (node->inputs[slot] != node->inputs[1]) break;
    }

    if (slot == node->inputlen) {
        return node->inputs[1];
    }

    return node;
}

B32 sea_is_const(SeaNode *node) {
    // TODO better type system
    return node->kind == SeaNodeKind_ConstInt;
}

SeaNode *sea_idealize_proj(SeaFunctionGraph *fn, SeaNode *node) {
    SeaNode *prev = node->inputs[0];

    switch (prev->kind) {
        case SeaNodeKind_If: {
            SeaNode *cond = prev->inputs[1];
            //
        } break;
    }


    return node;
}


SeaNode *sea_peephole(SeaFunctionGraph *fn, SeaNode *node) {
    node = sea_node_register(fn, node);

    node->type = sea_compute_type(fn, node);

    SeaNode *new = node;
    switch (node->kind) {
        case SeaNodeKind_Not:
        case SeaNodeKind_EqualI:
        case SeaNodeKind_NotEqualI:
        case SeaNodeKind_GreaterThanI:
        case SeaNodeKind_GreaterEqualI:
        case SeaNodeKind_LesserThanI:
        case SeaNodeKind_LesserEqualI:
        case SeaNodeKind_NegateI:
        case SeaNodeKind_AddI:
        case SeaNodeKind_SubI:
        case SeaNodeKind_MulI:
        case SeaNodeKind_DivI: {
            new = sea_idealize_int(fn, node);
        } break;

        case SeaNodeKind_Proj: {
            new = sea_idealize_proj(fn, node);
        } break;

        case SeaNodeKind_Phi: {
            new = sea_idealize_phi(fn, node);
        } break;
    }

    SeaNode *result = sea_dead_code_elim(fn, node, new);

    return result;
}



SeaNode *sea_create_const_int(SeaFunctionGraph *fn, S64 v) {
    SeaNode n = (SeaNode){
        .kind = SeaNodeKind_ConstInt,
        .vint = v,
    };

    sea_node_alloc_inputs(fn, &n, 1);
    sea_node_set_input(fn, &n, fn->start, 0);

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_urnary_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input) {
    SeaNode n = (SeaNode){
        .kind = kind,
    };

    sea_node_alloc_inputs(fn, &n, 1);
    sea_node_set_input(fn, &n, input, 0);

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_bin_op(
    SeaFunctionGraph *fn,
    SeaNodeKind op,
    SeaNode *lhs,
    SeaNode *rhs
) {

    // SeaDataType *t = sea_meet_type(lhs->type, rhs->type);

    SeaNode n = {
        .kind = op,
    };
    sea_node_alloc_inputs(fn, &n, 2);
    sea_node_set_input(fn, &n, lhs, 0);
    sea_node_set_input(fn, &n, rhs, 1);

    return sea_peephole(fn, &n);
}


SeaNode *sea_create_start(
    SeaFunctionGraph *fn,
    SeaFunctionProto proto
) {
    Assert(fn->scope);
    SeaNode *start = push_item(fn->arena, SeaNode);
    start->kind = SeaNodeKind_Start;

    U64 count = proto.args.count + 1;

    SeaType **types = push_array(fn->arena, SeaType *, count);
    types[0] = &sea_type_CtrlLive;
    for EachIndexFrom(i, 1, count) {
        types[i] = proto.args.fields[i-1].type;
    }

    start->type = sea_type_tuple(fn, types, count);

    SeaNode *ctrl = sea_create_proj(fn, start, 0);
    sea_insert_local_symbol(fn, CTRL_STR, ctrl);
    for EachIndexFrom(i, 1, count) {
        SeaNode *node = sea_create_proj(fn, start, i);
        sea_insert_local_symbol(fn, proto.args.fields[i-1].name, node);
    }



    return start;
}

SeaNode *sea_create_stop(SeaFunctionGraph *fn, U16 input_reserve) {
    SeaNode *stop = push_item(fn->arena, SeaNode);
    stop->kind = SeaNodeKind_Stop;
    stop->type = &sea_type_CtrlLive;
    sea_node_alloc_inputs(fn, stop, input_reserve);
}

SeaNode *sea_create_proj(SeaFunctionGraph *fn, SeaNode *input, U64 v) {
    SeaNode n = {
        .kind = SeaNodeKind_Proj,
        .vint = v,

    };

    // TODO move to compute
    sea_node_alloc_inputs(fn, &n, 1);
    sea_node_set_input(fn, &n, input, 0);

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr) {
    SeaNode n = {.kind = SeaNodeKind_Return};

    sea_node_alloc_inputs(fn, &n, 2);
    sea_node_set_input(fn, &n, prev_ctrl, 0);
    sea_node_set_input(fn, &n, expr, 1);

    SeaNode *final = sea_peephole(fn, &n);

    // add return to the stop node
    sea_node_append_input(fn, fn->stop, final);

    return final;
}

SeaNode *sea_create_loop(SeaFunctionGraph *fn, SeaNode *prev_ctrl) {
    SeaNode n = {
        .kind = SeaNodeKind_Loop,
    };

    sea_node_alloc_inputs(fn, &n, 3);
    sea_node_set_input(fn, &n, prev_ctrl, 1); // entry

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b) {
    SeaNode n = {.kind = SeaNodeKind_Phi};

    sea_node_alloc_inputs(fn, &n, 3);
    sea_node_set_input(fn, &n, region, 0);
    sea_node_set_input(fn, &n, a, 1);
    sea_node_set_input(fn, &n, b, 2);

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *ctrl, SeaNode *cond) {
    SeaNode n = {.kind = SeaNodeKind_If};



    sea_node_alloc_inputs(fn, &n, 2);
    sea_node_set_input(fn, &n, ctrl, 0);
    sea_node_set_input(fn, &n, cond, 1);

    sea_node_alloc_users(fn, &n, 2);

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_region(SeaFunctionGraph *fn, SeaNode **ctrl_ins, U16 ctrl_count, U16 output_reserves) {
    SeaNode n = {.kind = SeaNodeKind_Region};

    sea_node_alloc_inputs(fn, &n, ctrl_count);
    for EachIndex(i, ctrl_count) {
        sea_node_set_input(fn, &n, ctrl_ins[i], i);
    }

    if (output_reserves) sea_node_alloc_users(fn, &n, output_reserves);

    return sea_peephole(fn, &n);
}
