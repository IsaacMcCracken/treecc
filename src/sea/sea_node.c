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
    node->inputs = push_array(fn->arena, SeaNode*, cap);
    node->inputcap = cap;
}


void sea_node_append_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot) {
    SeaUser *user_node = push_item(fn->arena, SeaUser);
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
        SeaNode **new_inputs = push_array(fn->arena, SeaNode*, newcap);
        MemoryCopyTyped(new_inputs, node->inputs,node->inputlen);
        node->inputs = new_inputs;
        node->inputcap = newcap;
    }

    U16 slot = node->inputlen;
    node->inputs[node->inputlen] = input;
    node->inputlen += 1;

    if (input) {
        sea_node_append_user(fn, input, node, slot);
    }

    return slot;
}


void sea_node_remove_input(SeaFunctionGraph *fn, SeaNode *node, U16 slot) {
    SeaNode *old = node->inputs[slot];
    if (old) {
        sea_node_remove_user(fn, old, node);
        if (old->users == 0) sea_node_kill(fn, old);
    }

    if (slot < node->inputlen - 1) {
        //TODO identity changed

        // get last element
        SeaNode *tmp = node->inputs[node->inputlen - 1];
        node->inputs[slot] = tmp;
    }

    node->inputlen -= 1;
}

void sea_node_remove_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user) {
    SeaUser *curr = node->users;
    SeaUser *prev = 0;
    while (curr) {
        SeaNode *curr_user = sea_user_val(curr);
        if  (curr_user == user) {
            if (prev) {
                prev->next = curr->next;
                // TODO maybe free user struct
            } else {
                node->users = curr->next;
            }
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    fprintf(stderr, "User()  not in node.\n");
    Trap();
}

void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot) {
    assert(slot < node->inputcap);

    // todo this changes the identiy of the node.
    // so remove it from sea_node_map and put it back in with its new identity;


    if (node->inputs[slot]) {
        sea_node_remove_user(fn, node->inputs[slot], node);
    }

    if (input) sea_node_append_user(fn, input, node, slot);

    node->inputlen = (node->inputlen > slot) ? node->inputlen : slot + 1; // this bad boy cause a big bug love those off by one errors
    node->inputs[slot] = input;

}

SeaNode *sea_node_singleton(SeaFunctionGraph *fn, SeaNode *node) {
    SeaNode *canonical = sea_map_lookup(&fn->map, node);
    if (canonical) {
        return canonical;
    }

    canonical = push_item(fn->arena, SeaNode);
    MemoryCopyStruct(canonical, node);

    sea_map_insert(&fn->map, canonical);

    for EachIndex(i, canonical->inputlen) {
        SeaNode *input = canonical->inputs[i];
        if (input) {
            sea_node_append_user(fn, input, canonical, i);
        }
    }

    return canonical;
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
    sea_node_remove_user(fn, node, 0);
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
    // garbage collection info
    fn->deadspace += sizeof(SeaNode);
    fn->deadspace += sizeof(SeaNode*) * node->inputcap;
    // TODO if there is extra stuff allocated for special nodes add to deadspace

    // todo copy graph is there is too much deadspace

    // remove from hash map (gvn)
    sea_map_remove(&fn->map, node);

    // remove user from inputs
    for EachIndex(i, node->inputlen) {
        SeaNode *input = node->inputs[i];
        if (input) {
            sea_node_remove_user(fn, input, node);
            if (input->users == 0) sea_node_kill(fn, input);
        }
    }

    node->type = 0;
    node->inputlen = 0;
    node->kind = SeaNodeKind_Invalid;
}

void sea_node_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new) {
    Assert(old != new);
    for EachNode(user_node, SeaUser, old->users) {
        SeaNode *user = sea_user_val(user_node);
        U16 slot = sea_user_slot(user_node);
        sea_node_set_input(fn, user, new, slot);

    }
    sea_node_kill(fn, old);
}

SeaNode *sea_dead_code_elim(SeaFunctionGraph *fn, SeaNode *this, SeaNode *m) {
    if (this != m && this->users == 0) {
        sea_node_kill(fn, this);
    }

    return m;
}

B32 sea_node_is_cgf(SeaNode *node) {
    switch (node->kind) {
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop:
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
    if (sea_node_is_cgf(node)) return 0;
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
    SeaNode n = (SeaNode){
        .kind = SeaNodeKind_ConstInt,
        .vint = v,
    };

    sea_node_alloc_inputs(fn, &n, 1);
    n.inputs[0] = fn->start;
    n.inputlen = 1;

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_urnary_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input) {
    SeaNode n = (SeaNode){
        .kind = kind,
    };

    sea_node_alloc_inputs(fn, &n, 2);
    n.inputs[1] = input;
    n.inputlen = 2;

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
    sea_node_alloc_inputs(fn, &n, 3);
    n.inputs[1] = lhs;
    n.inputs[2] = rhs;
    n.inputlen = 3;

    return sea_peephole(fn, &n);
}


SeaNode *sea_create_start(
    SeaFunctionGraph *fn,
    SeaScopeManager *m,
    SeaFunctionProto proto
) {
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
    sea_scope_insert_symbol(fn, m, CTRL_STR, ctrl);

    for EachIndexFrom(i, 1, count) {
        SeaNode *node = sea_create_proj(fn, start, i);
        sea_scope_insert_symbol(fn, m, proto.args.fields[i-1].name, node);
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

    // TODO figure out input layout
    sea_node_alloc_inputs(fn, &n, 1);
    n.inputs[0] = input;
    n.inputlen = 1;
    return sea_peephole(fn, &n);
}

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr) {
    SeaNode n = {.kind = SeaNodeKind_Return};

    sea_node_alloc_inputs(fn, &n, 2);

    n.inputs[0] = prev_ctrl;
    n.inputs[1] = expr;
    n.inputlen = 2;

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
    n.inputs[1] = prev_ctrl; // entry
    n.inputlen = 3;

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b) {
    SeaNode n = {.kind = SeaNodeKind_Phi};

    sea_node_alloc_inputs(fn, &n, 3);
    n.inputs[0] = region;
    n.inputs[1] = a;
    n.inputs[2] = b;
    n.inputlen = 3;

    return sea_peephole(fn, &n);
}

SeaNode *sea_create_phi(SeaFunctionGraph *fn, SeaNode **inputs, U16 count) {
    SeaNode n = {.kind = SeaNodeKind_Phi};
    sea_node_alloc_inputs(fn, &n, count);
    for EachIndex(i, count) {
        n.inputs[i] = inputs[i];
    }

    n.inputlen = count;
   return sea_peephole(fn, &n);
}

SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *ctrl, SeaNode *cond) {
    SeaNode n = {.kind = SeaNodeKind_If};



    sea_node_alloc_inputs(fn, &n, 2);
    n.inputs[0] = ctrl;
    n.inputs[1] = cond;
    n.inputlen = 2;


    return sea_peephole(fn, &n);
}


SeaNode *sea_create_scope(SeaScopeManager *m, U16 input_reserve) {
    SeaNode *n = push_item(m->arena, SeaNode);
    n->kind = SeaNodeKind_Scope,
    n->vptr = push_item(m->arena, SeaScopeList);
    n->inputs = push_array(m->arena, SeaNode*, input_reserve);
    n->inputcap = input_reserve;
    return n;
}

SeaNode *sea_create_region(SeaFunctionGraph *fn, SeaNode **inputs, U16 ctrl_count) {
    SeaNode n = {.kind = SeaNodeKind_Region};


    sea_node_alloc_inputs(fn, &n, ctrl_count);
    for EachIndex(i, ctrl_count) {
        n.inputs[i] = inputs[i];
        // printf("%p ",n.inputs[i]);
    }
    // printf("\n");
    n.inputlen = ctrl_count;


    return sea_node_singleton(fn, &n);
}
