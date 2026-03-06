#include "sea_internal.h"


B32 is_multihead(SeaNode *n) {
    return n->kind == SeaNodeKind_Start || n->kind == SeaNodeKind_If;
}

B32 is_multitail(SeaNode *n) {
    return is_multihead(n->inputs[0]);
}

void sea_node_arr_push(SeaNode **arr, U16 *len, U16 cap, SeaNode *node) {
    Assert(*len < cap);
    arr[(*len)++] = node;
}

SeaNode *sea_node_arr_remove(SeaNode **arr, U16 *len, U16 cap, U16 idx) {
    Assert(idx < *len);
    SeaNode *n = arr[idx];
    arr[idx] = arr[--(*len)];
    arr[*len] = 0;
    return n;
}

U16 node_sched_score(SeaNode *n) {
    if (is_multitail(n)) return 1001;
    if (sea_node_is_cfg(n)) {
        return n->kind == SeaNodeKind_Return ? 2 : 1;
    }

    return 500;
}

// U16 pick_best_node(SeaNode **unordered, U16 len, )


void local_schedule(U16 *bcnts, SeaNode *bb) {
    if (!node_is_blockhead(bb) || bb->kind == SeaNodeKind_Stop)
        return;

    U64 bb_len = 0;
    // count users in local block
    for EachNode(user_node, SeaUser, bb->users) {
        bb_len += 1;
        SeaNode *user = sea_user_val(user_node);
        if (user->inputs[0] == bb && user->kind != SeaNodeKind_Phi) {
            for EachIndex(i, user->inputlen) {
                SeaNode *in = user->inputs[i];
                if (in && cfg_zero(in) == bb) {
                    bcnts[user->nid] += 1;
                }
            }
        }
    }

    Temp scratch = scratch_begin(0, 0);

    U16 cap = bb_len;
    // Use array because my chud ass fucking sucks
    SeaNode **schedule  = push_array(scratch.arena, SeaNode*, cap);
    U16 schedule_len = 0;


    for EachNode(user_node, SeaUser, bb->users) {
        SeaNode *user = sea_user_val(user_node);
        if (bcnts[user->nid] == 0) {
            sea_node_arr_push(schedule, &schedule_len, cap, user);
        }
    }


    // todo fix this chud ass scheduling
    // n ^2 but idk
    U16 begin = 0;
    while (begin < bb_len) {
        SeaNode *n = schedule[begin];
        for EachNode(user_node, SeaUser, n->users) {
            SeaNode *user = sea_user_val(user_node);
            // if the user is in basic block
            U16 c = bcnts[user->nid]; // c > 0 if not scheduled
            // if in block and not scheduled
            if (cfg_zero(user) == bb && c)  {

                bcnts[user->nid] -= 1; // it has one less unscheduled user in the block
                if (bcnts[user->nid] == 0) // scheule it if all depencies are scheduled
                    sea_node_arr_push(schedule, &schedule_len, cap, user);
            }
        }
        begin++;
    }


    // todo get this out of here somehow

    for EachIndex(i, schedule_len) {
        SeaNode *n = schedule[i];
        String8 opcode = sea_node_instr_label(scratch.arena, n);
        printf("%%%d %.*s ", (int)n->nid, str8_varg(opcode));
        scratch_end(scratch);
        for EachIndexFrom(i, 1, n->inputlen) {
            printf("%%%d ", n->inputs[i]->nid);
        }
        printf("\n");
    }

    scratch_end(scratch);
}

void local_sched_walk(BitArray *visit, U16 *bcnts, SeaNode *cfg) {
    if (bits_get(visit, cfg->nid)) return;
    bits_set(visit, cfg->nid);

    local_schedule(bcnts, cfg);
    for EachNode(user_node, SeaUser, cfg->users) {
        SeaNode *user = sea_user_val(user_node);
        if (sea_node_is_cfg(user)) local_sched_walk(visit, bcnts, user);
    }
}

void sea_list_schedule(SeaFunctionGraph *fn) {
    Temp scratch = scratch_begin(0, 0);

    // TODO Might want a actual allocator
    // I think this means block counts???
    U16 *bcnts = push_array(scratch.arena, U16, fn->nidcap);
    BitArray visit = bits_alloc(scratch.arena, fn->nidcap);

    local_sched_walk(&visit, bcnts, fn->start);


    scratch_end(scratch);
}
