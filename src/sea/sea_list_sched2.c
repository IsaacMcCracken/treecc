#include "sea_internal.h"




B32 node_starts_bb(SeaNode *n) {
    switch (n->kind) {
        case SeaNodeKind_Start:
        case SeaNodeKind_Region:
        case SeaNodeKind_Loop:
            return 1;
        case SeaNodeKind_Proj:
            return sea_node_is_cfg(n) && (n->inputs[0]->kind != SeaNodeKind_Start);
    }
    return 0;
}

B32 node_ends_bb(SeaNode *n) {
    switch (n->kind) {
        case SeaNodeKind_If:
        case SeaNodeKind_Return:
            return 1;
    }

    return 0;
}



void pretty_print(SeaFunctionGraph *fn) {
    Temp scratch = scratch_begin(0, 0);

    U16 *ids = push_array(scratch.arena, U16, fn->nidcap);
    // set ids
    int id = 0, bbid = 0;
    for EachNode(bb, SeaBlock, fn->blocks.head) {
        ids[bb->begin->nid] = bbid;
        bbid += 1;
        for EachIndex(i, bb->nodelen) {
            SeaNode *n = bb->nodes[i];
            ids[n->nid] = id;
            id += 1;
        }
    }

    // print
    for EachNode(bb, SeaBlock, fn->blocks.head) {
        String8 bblabel = sea_node_label(scratch.arena, bb->begin);
        printf("BB%d_%.*s:\n", ids[bb->begin->nid], str8_varg(bblabel));

        for EachIndex(i, bb->nodelen) {
            SeaNode *n = bb->nodes[i];
            String8 label = sea_node_label(scratch.arena, n);
            printf("    v%d %.*s ", ids[n->nid], str8_varg(label));

            int start = sea_node_is_cfg(n) ? 0 : 1;
            for EachIndexFrom(j, start, n->inputlen) {
                SeaNode *in = n->inputs[j];
                int in_id = ids[in->nid];
                if (node_starts_bb(in)) {
                    String8 in_label = sea_node_label(scratch.arena, in);
                    printf("BB%d_%.*s ", in_id, str8_varg(in_label));
                } else {
                    printf("v%d ", in_id);
                }
            }
            printf("\n");
        }
    }


    scratch_end(scratch);

}


B32 is_back_edge(SeaNode *src, SeaNode *dst) {
    return dst->inputlen > 2
        && dst->inputs[2] == src
        && (dst->kind == SeaNodeKind_Loop ||
           (dst->kind == SeaNodeKind_Phi && dst->inputs[0]->kind == SeaNodeKind_Loop));
}

void get_bbs_postorder_style(SeaFunctionGraph *fn, BitArray *visit, SeaNode *n) {
    if (!n || bits_get(visit, n->nid)) return;
    if (!sea_node_is_cfg(n)) return;
    bits_set(visit, n->nid);

    // recurse FIRST
    for EachIndex(i, n->inputlen) {
        SeaNode *in = n->inputs[i];
        if (!in || !sea_node_is_cfg(in)) continue;
        get_bbs_postorder_style(fn, visit, in);
    }

    // THEN push after children are done = postorder
    if (node_starts_bb(n)) {
        SeaBlock *bb = sea_alloc_item(fn, SeaBlock);
        bb->begin = n;
        DLLPushBack(fn->blocks.head, fn->blocks.tail, bb);
    }
}

void sched_node(SeaFunctionGraph *fn, SeaBlock *bb, U16 *dep_data, SeaNode *n) {
    Assert(fn->schedlen < fn->schedcap);

    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        if (cfg_zero(user) == bb->begin) {
            dep_data[user->nid] -= 1;
        }
    }

    fn->sched[fn->schedlen] = n;
    fn->schedlen += 1;
    bb->nodelen += 1;
}

void sched_bb(SeaFunctionGraph *fn, SeaBlock *bb, U16 *dep_data) {
    SeaNode *block_head = bb->begin;

    fn->sched[fn->schedlen] = block_head;
    fn->schedlen += 1;

    SeaNode **nodes = &fn->sched[fn->schedlen];
    bb->nodes = nodes;

    U16 block_size = 0;
    for EachNode(user_node, SeaUser, bb->begin->users) {
        SeaNode *user = sea_user_val(user_node);

        // is node sheduled in this block
        if (cfg_zero(user) == block_head) {
            block_size += 1;
            // for each depency of user if they are in they
            // the same basic block add to the depency count
            for EachIndexFrom(i, 1, user->inputlen) {
                SeaNode *in = user->inputs[i];
                if (cfg_zero(in) == block_head) {
                    dep_data[user->nid] += 1;
                }
            }
        }
    }

    if (block_size == 0) return;

    Temp scratch = scratch_begin(0, 0);
    SeaNode **unsched = push_array(scratch.arena, SeaNode *, block_size);
    U16 unschedlen = 0;
    SeaNode **ready = push_array(scratch.arena, SeaNode *, block_size);
    U16 readylen = 0;

    for EachNode(user_node, SeaUser, bb->begin->users) {
        SeaNode *user = sea_user_val(user_node);
        if (cfg_zero(user) == block_head) {
            if (dep_data[user->nid] == 0) {
                ready[readylen++] = user;
            } else {
                unsched[unschedlen++] = user;
            }
        }
    }

    Assert(readylen > 0);

    // drain
    while (readylen > 0) {
        SeaNode *n = ready[--readylen];  // or [0] with a shift for FIFO
        sched_node(fn, bb, dep_data, n);
        // check if any unsched nodes became ready
        for (U16 i = 0; i < unschedlen; i++) {
            if (unsched[i] && dep_data[unsched[i]->nid] == 0) {
                ready[readylen++] = unsched[i];
                unsched[i] = 0;
            }
        }
    }

    // Assert(unschedlen == 0);


    scratch_end(scratch);

}

void sea_local_schedule(SeaFunctionGraph *fn) {
    // get blocks
    {
        Temp scratch = scratch_begin(0,0);
        BitArray visit = bits_alloc(scratch.arena, fn->nidcap);
        get_bbs_postorder_style(fn, &visit, fn->stop);
        scratch_end(scratch);
    }

    // topo sort
    {
        // Alloc Schedule Array
        U64 cap = 2 * AlignPow2(fn->node_count, 64);
        SeaNode **nodes = sea_alloc_array(fn, SeaNode*, cap);
        fn->schedcap = cap;
        fn->sched = nodes;
        Temp scratch = scratch_begin(0,0);

        U16 *dep_data = push_array(scratch.arena, U16, fn->nidcap);

        for EachNode(bb, SeaBlock, fn->blocks.head) {
            sched_bb(fn, bb, dep_data);
        }

        scratch_end(scratch);
    }

    // print
    {
        pretty_print(fn);
    }
}
