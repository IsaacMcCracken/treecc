#include "sea_internal.h"


SeaNode *gen_2addr_move(SeaFunctionGraph *fn, SeaNode *n) {
    SeaNode *in = n->inputs[1];
    SeaNode *mv = sea_node_alloc(fn, SeaNodeKind_Move, 2, 2);
    sea_node_set_input(fn, mv, in, 1);
    sea_node_set_input(fn, n, mv, 1);
    return mv;
}

SeaNode *sea_gen_copy(SeaFunctionGraph *fn, SeaNode *phi, U16 idx) {
    SeaNode *region = phi->inputs[0];
    SeaNode *n = sea_node_alloc(fn, SeaNodeKind_Copy, 2, 2);
    sea_node_set_input(fn, n, region->inputs[idx], 0);
    sea_node_set_input(fn, n, phi->inputs[idx], 1);

    // set the phi's input to the copy
    sea_node_set_input(fn, phi, n, idx);

    return n;
}

void dessa_walk(SeaFunctionGraph *fn, BitArray *visit, SeaNode *n) {
    if (!n || bits_get(visit, n->nid) || !sea_node_is_cfg(n)) return;
    bits_set(visit, n->nid);

    if (is_2addr(n)) {
        if (sea_node_count_users(n) > 1) {
            gen_2addr_move(fn, n);
        }
    }

    for EachIndex(i, n->inputlen) {
        SeaNode *in = n->inputs[i];
        dessa_walk(fn, visit, in);
    }

    if (n->kind == SeaNodeKind_Loop || n->kind == SeaNodeKind_Region) {
        for EachNode(user_node, SeaUser, n->users) {
            SeaNode *phi = sea_user_val(user_node);
            if (phi->kind == SeaNodeKind_Phi) {
                for EachIndexFrom(i, 1, phi->inputlen) {
                    SeaNode *copy = sea_gen_copy(fn, phi, i);
                }
            }
        }
    }
}


void sea_ssa_deconstruction(SeaFunctionGraph *fn) {
    Temp scratch = scratch_begin(0, 0);
    BitArray visit = bits_alloc(scratch.arena, fn->nidcap);
    dessa_walk(fn, &visit, fn->stop);
    scratch_end(scratch);
}
