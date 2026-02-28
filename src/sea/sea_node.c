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




U32 sea_node_hash(SeaNode *node) {
    U64 h = 0xcbf29ce484222325ULL;

    h ^= (U64)node->kind;
    h *= 0x00000100000001b3ULL;

    h ^= (U64)node->vint;
    h *= 0x00000100000001b3ULL;

    for (U32 i = 0; i < node->inputlen; i++) {
        U64 ptr = (U64)node->inputs[i];
        ptr ^= ptr >> 3;
        h ^= ptr;
        h *= 0x00000100000001b3ULL;
    }

    // avalanche then fold 64->32
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    // XOR fold the two halves
    return (U32)(h ^ (h >> 32));
}

B32 sea_node_equal(SeaNode *a, SeaNode *b) {
    if (a->inputlen != b->inputlen) return 0;
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
        case SeaNodeKind_Proj:
        case SeaNodeKind_ConstInt:
        if (a->vint != b->vint) return 0;
    }


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
    node->inputs = sea_alloc_array(fn, SeaNode*, cap);
    node->inputcap = cap;
}

SeaNode *sea_node_alloc(SeaFunctionGraph *fn, SeaNodeKind kind, U16 inputcap, U16 inputlen) {
    SeaNode *node = sea_alloc_item(fn, SeaNode);
    node->kind = kind;
    node->inputlen = inputlen;
    if (inputcap)
        sea_node_alloc_inputs(fn, node, inputcap);
    return node;
}

void sea_node_append_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot) {
    SeaUser *user_node = sea_alloc_item(fn, SeaUser);
    user_node->n = (U64)user;
    user_node->slot = (U64)slot;
    SLLStackPush(node->users, user_node);
}

SeaNode *sea_user_val(SeaUser *user) {
    U64 val = user->n;
    return (SeaNode*)(val);
}

U16 sea_user_slot(SeaUser *user) {
    return user->slot;
}

U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input) {
    if (node->inputs == 0) {
        sea_node_alloc_inputs(fn, node, 4);
    }
    if (node->inputlen >= node->inputcap) {
        U16 newcap = node->inputcap * 2;
        SeaNode **new_inputs = sea_alloc_array(fn, SeaNode*, newcap);
        MemoryCopyTyped(new_inputs, node->inputs, node->inputlen);
        node->inputs = new_inputs;
        node->inputcap = newcap;
    }
    U16 slot = node->inputlen;
    node->inputlen += 1;
    sea_node_set_input(fn, node, input, slot);
    return slot;
}


void sea_node_remove_input(SeaFunctionGraph *fn, SeaNode *node, U16 slot) {
    sea_node_set_input(fn, node, 0, slot); // removes slot and user
    if (slot < node->inputlen - 1) {
        SeaNode *tmp = node->inputs[node->inputlen - 1]; // copy it for now
        sea_node_keep(fn, tmp);
        sea_node_set_input(fn, node, 0, node->inputlen - 1); // remove the input from
        sea_node_set_input(fn, node, tmp, slot);
        sea_node_unkeep(fn, tmp);
    }
    node->inputlen -=   1;
}


void sea_node_remove_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot) {
    SeaUser *curr = node->users;
    SeaUser *prev = 0;
    while (curr) {
        SeaNode *curr_user = sea_user_val(curr);
        U16 curr_slot = sea_user_slot(curr);
        if  (curr_user == user && curr_slot == slot) {
            if (prev) prev->next = curr->next;
            else node->users = curr->next;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    fprintf(stderr, "Looking for user=%p slot=%u in node=%p\n", user, slot, node);
    for (SeaUser *c = node->users; c; c = c->next) {
        fprintf(stderr, "  found user=%p slot=%u\n", sea_user_val(c), sea_user_slot(c));
    }
    Trap();
}

void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot) {
    assert(slot < node->inputcap);

    // todo this changes the identiy of the node.
    // so remove it from sea_node_map and put it back in with its new identity;
    SeaNode *old = node->inputs[slot];
    if (input == old) return;

    if (old) {
        sea_node_remove_user(fn, old, node, slot);
        if (old->users == 0) sea_node_kill(fn, old);
    }

    if (input) sea_node_append_user(fn, input, node, slot);

    node->inputs[slot] = input;

}



SeaNode *sea_node_idom(SeaFunctionGraph *fn, SeaNode *node) {
    switch (node->kind) {
        case SeaNodeKind_Region: {
            Assert(node->inputlen == 2);
            SeaNode *lhs = sea_node_idom(fn, node->inputs[1]);
            SeaNode *rhs = sea_node_idom(fn, node->inputs[2]);
            while (lhs != rhs) {
                if (lhs == 0 || rhs == 0) return 0;
                S32 lidepth = lhs->idepth;
                S32 ridepth = rhs->idepth;
                S32 cmp = lidepth - ridepth;
                if (cmp >= 0) lhs = sea_node_idom(fn, lhs);
                if (cmp <= 0) rhs = sea_node_idom(fn, rhs);
            }
            if (lhs == 0) return 0;
            node->idepth = lhs->idepth + 1;
            return lhs;
        } break;
        default: {
            SeaNode *idom = node->inputs[0];
            if (idom) {
                if (idom->idepth == 0) sea_node_idom(fn, idom);
                node->idepth = idom->idepth + 1;
            } else {
                node->idepth = 1;
            }
            return idom;
        } break;
    }
}



void sea_node_keep(SeaFunctionGraph *fn, SeaNode *node) {
    sea_node_append_user(fn, node, 0, 0);
}

void sea_node_unkeep(SeaFunctionGraph *fn, SeaNode *node) {
    sea_node_remove_user(fn, node, 0, 0);
}


void sea_node_detroy(SeaFunctionGraph *fn, SeaNode *node) {
    for EachIndex(i, node->inputlen) {
        sea_node_set_input(fn, node, 0, i);
    }
    // for EachIndex(i, node->userlen) {
    //     SeaNode *user = node->users[i].n;
    //     U16 slot = old->users[i].slot;
    //     sea_node_set_input(fn, user, 0, slot);
    // }
}

void sea_node_kill(SeaFunctionGraph *fn, SeaNode *node) {
    Assert(node->users == 0);
    Assert(node->kind != SeaNodeKind_Start);

    // Assert(node->vint != 32);


    for EachIndex(i, node->inputlen) {
        sea_node_set_input(fn, node, 0, i);
    }
    node->type = 0;
    node->inputlen = 0;
    node->kind = SeaNodeKind_Invalid;
}

void sea_node_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new) {
    Assert(old != new);
    sea_node_keep(fn, new);
    while (old->users) {
        SeaUser *user_node = old->users;
        SeaNode *user = sea_user_val(user_node);
        U16 slot = sea_user_slot(user_node);
        // sea_node_set_input will call sea_node_remove_user on old,
        // which pops user_node off old->users — so we don't advance manually
        sea_node_set_input(fn, user, new, slot);
    }
    sea_node_kill(fn, old);
    sea_node_unkeep(fn, new);
}

SeaNode *sea_dead_code_elim(SeaFunctionGraph *fn, SeaNode *this, SeaNode *m) {
    if (this != m && this->users == 0) {
        sea_node_keep(fn, m);
        sea_node_kill(fn, this);
        sea_node_unkeep(fn, m);
    }

    return m;
}

B32 sea_node_is_cfg(SeaNode *node) {
    switch (node->kind) {
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop:
        case SeaNodeKind_Dead:
        case SeaNodeKind_Return:
        case SeaNodeKind_If:
        case SeaNodeKind_Region:
        case SeaNodeKind_Loop:
            return 1;
        case SeaNodeKind_Proj: {
            SeaNode *ctrl = node->inputs[0];
            return node->vint == 0 || ctrl->kind == SeaNodeKind_If;
        }
    }

    return 0;
}

B32 sea_node_is_op(SeaNode *node) {
    if (sea_node_is_cfg(node)) return 0;
    Assert(node->kind != SeaNodeKind_Scope);
    switch (node->kind) {
        case SeaNodeKind_ConstInt:
        case SeaNodeKind_Proj:
        case SeaNodeKind_Phi:
            return 0;
    }
    return 1;
}

B32 sea_node_is_bin_op(SeaNode *node) {
    if (!sea_node_is_op(node)) return 0;
    switch (node->kind) {
        case SeaNodeKind_Not:
        case SeaNodeKind_NegateI:
        case SeaNodeKind_BitNotI:
            return 0;
    }
    return 1;
}

B32 sea_node_is_urnary_op(SeaNode *node) {
    if (!sea_node_is_op(node)) return 0;
    switch (node->kind) {
        case SeaNodeKind_Not:
        case SeaNodeKind_NegateI:
        case SeaNodeKind_BitNotI:
            return 1;
    }
    return 0;
}


SeaNode *sea_create_const_int(SeaFunctionGraph *fn, S64 v) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_ConstInt, 1, 1);
    sea_node_set_input(fn, node, fn->start, 0);
    node->vint = v;
    return sea_peephole(fn, node);
}

SeaNode *sea_create_urnary_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input) {
    SeaNode *node = sea_node_alloc(fn, kind, 2, 2);
    sea_node_set_input(fn, node, input, 1);
    return sea_peephole(fn, node);
}

SeaNode *sea_create_bin_op(
    SeaFunctionGraph *fn,
    SeaNodeKind op,
    SeaNode *lhs,
    SeaNode *rhs
) {
    SeaNode *node = sea_node_alloc(fn, op, 3, 3);
    sea_node_set_input(fn, node, lhs, 1);
    sea_node_set_input(fn, node, rhs, 2);

    return sea_peephole(fn, node);
}


SeaNode *sea_create_start(
    SeaFunctionGraph *fn,
    SeaScopeManager *m,
    SeaFunctionProto proto
) {
    SeaNode *start = sea_node_alloc(fn, SeaNodeKind_Start, 0, 0);

    U64 count = proto.args.count + 1;

    SeaType **types = sea_alloc_array(fn, SeaType *, count);
    types[0] = &sea_type_CtrlLive;
    for EachIndexFrom(i, 1, count) {
        types[i] = proto.args.fields[i-1].type;
    }

    start->type = sea_type_tuple(fn, types, count);

    SeaNode *ctrl = sea_create_proj(fn, start, 0);
    sea_scope_insert_symbol(fn, m, CTRL_STR, ctrl);
    Assert(m->curr->inputlen == 1);

    for EachIndexFrom(i, 1, count) {
        SeaNode *node = sea_create_proj(fn, start, i);
        sea_scope_insert_symbol(fn, m, proto.args.fields[i-1].name, node);
    }



    return start;
}

void sea_node_set_ctrl(SeaFunctionGraph *fn, SeaNode *node, SeaNode *ctrl) {
    sea_node_keep(fn, ctrl);
    sea_node_set_input(fn, node, ctrl, 0);
    sea_node_unkeep(fn, ctrl);
}

SeaNode *sea_node_get_ctrl(SeaFunctionGraph *fn, SeaNode *node) {
    return node->inputs[0];
}


SeaNode *sea_create_stop(SeaFunctionGraph *fn, U16 input_reserve) {
    SeaNode *stop = sea_node_alloc(fn, SeaNodeKind_Stop, input_reserve, 0);
    stop->type = &sea_type_CtrlLive;
    return stop;
}

SeaNode *sea_create_proj(SeaFunctionGraph *fn, SeaNode *input, U64 v) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_Proj, 1, 1);
    node->vint = v;
    sea_node_set_input(fn, node, input, 0);
    return sea_peephole(fn, node);
}

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_Return, 2, 2);
    sea_node_set_input(fn, node, prev_ctrl, 0);
    sea_node_set_input(fn, node, expr, 1);

    SeaNode *final = sea_peephole(fn, node);
    return final;
}

SeaNode *sea_create_loop(SeaFunctionGraph *fn, SeaNode *prev_ctrl) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_Loop, 3, 3);
    sea_node_set_input(fn, node, prev_ctrl, 1);
    return sea_peephole(fn, node);
}


SeaNode *sea_create_phi(SeaFunctionGraph *fn, SeaNode **inputs, U16 count) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_Phi, count, count);
    for EachIndex(i, count) {
        sea_node_set_input(fn, node, inputs[i], i);
    }
    return node;
}

SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b) {
    SeaNode *inputs[] = {region, a, b};
    return sea_create_phi(fn, inputs, 3);
}

SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *ctrl, SeaNode *cond) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_If, 2, 2);
    sea_node_set_input(fn, node, ctrl, 0);
    sea_node_set_input(fn, node, cond, 1);
    return sea_peephole(fn, node);
}


SeaNode *sea_create_scope(SeaScopeManager *m, U16 input_reserve) {
    SeaNode *n = push_item(m->arena, SeaNode);
    n->kind = SeaNodeKind_Scope,
    n->vptr = push_item(m->arena, SeaScopeList);
    n->inputs = push_array(m->arena, SeaNode*, input_reserve);
    n->inputcap = input_reserve;
    return n;
}

void sea_add_return(SeaFunctionGraph *fn, SeaNode *ret) {
    if (ret->kind == SeaNodeKind_Return)
        sea_node_append_input(fn, fn->stop, ret);
}

SeaNode *sea_create_dead_ctrl(SeaFunctionGraph *fn) {
    return sea_peephole(fn, sea_node_alloc(fn, SeaNodeKind_Dead, 0, 0));
}

SeaNode *sea_create_region(SeaFunctionGraph *fn, SeaNode **inputs, U16 ctrl_count) {
    SeaNode *node = sea_node_alloc(fn, SeaNodeKind_Region, ctrl_count, ctrl_count);

    for EachIndex(i, ctrl_count) {
        sea_node_set_input(fn, node, inputs[i], i);
    }

    return node;
}
