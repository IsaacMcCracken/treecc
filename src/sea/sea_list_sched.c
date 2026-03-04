#include "sea_internal.h"


void local_schedule(U16 *bcnts, SeaNode *bb) {
    if (!node_is_blockhead(bb) || bb->kind == SeaNodeKind_Stop)
        return;

    // count users in local block
    for EachNode(user_node, SeaUser, bb->users) {
        SeaNode *user = sea_user_val(user_node);
        if (user->inputs[0] == bb && user->kind != SeaNodeKind_Phi) {
            // original is
            // for( Node def : use._inputs )
            // Has defs from same block?  Then block-local inputs
            // if( def != null && def.cfg0()==bb ) // Same block?
            // BCNTS[use._nid]++;     // Raise block-local input count
            for EachIndex(i, user->inputlen) {
                SeaNode *in = user->inputs[i];
                if (in && in->inputs[0] == bb)
                    bcnts[user->nid] += 1;
            }
        }
    }

    Temp scratch = scratch_begin(0, 0);

    SeaNodeList ready_queue = { 0 };

    for EachNode(user_node, SeaUser, bb->users) {
        SeaNode *user = sea_user_val(user_node);
        if (bcnts[user->nid] == 0) {
            SeaNodeNode *nn = push_item(scratch.arena, SeaNodeNode);
            nn->node = user;
            sea_node_list_push_tail(&ready_queue, nn);
        }
    }

    Assert(ready_queue.count > 0);




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

void list_scheduler(SeaFunctionGraph *fn) {
    Temp scratch = scratch_begin(0, 0);

    // TODO Might want a actual allocator
    // I think this means block counts???
    U16 *bcnts = push_array(scratch.arena, U16, fn->nidcap);
    BitArray visit = bits_alloc(scratch.arena, fn->nidcap);



    scratch_end(scratch);
}
