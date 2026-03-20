#include "sea_internal.h"

SeaNode *sea_node_lca(SeaNode *lhs, SeaNode *rhs);

SeaNode *sea_node_idom(SeaNode *node) {
    switch (node->kind) {
        case SeaNodeKind_Region: {
            SeaNode *lca = 0;
            for EachIndex(i, node->inputlen) {
                lca = sea_node_lca(node->inputs[i], lca);
            }
            return lca;
        } break;
        case SeaNodeKind_Loop: {
            return node->inputs[1];
        } break;
        case SeaNodeKind_Stop:
        case SeaNodeKind_Start: {
            return 0;
        } break;
        default: {
            SeaNode *idom = node->inputs[0];
            if (idom) {
                if (idom->idepth == 0) sea_node_idom(idom);
                node->idepth = idom->idepth + 1;
            } else {
                node->idepth = 1;
            }
            return idom;
        } break;
    }
}

U16 sea_node_idepth(SeaNode *n) {
    if (n->idepth != 0) return n->idepth;
    switch (n->kind) {
        case SeaNodeKind_Start: return 0;
        case SeaNodeKind_Stop: {
            U16 d = 0;
            for EachIndex(i, n->inputlen) {
                SeaNode *in = n->inputs[i];
                d = Max(d, sea_node_idepth(in) + 1);
            }
            n->idepth = d;
            return d;
        }
        default: {
            SeaNode *dom = sea_node_idom(n);
            U16 d = sea_node_idepth(dom) + 1;
            n->idepth = d;
            return d;
        }
    }

    return n->idepth;
}

SeaNode *sea_node_lca(SeaNode *lhs, SeaNode *rhs) {
    if (!rhs) return lhs;
    while (lhs != rhs) {
        S32 lidepth = sea_node_idepth(lhs);
        S32 ridepth = sea_node_idepth(rhs);
        S32 cmp = lidepth - ridepth;
        if (cmp >= 0) lhs = sea_node_idom(lhs);
        if (cmp <= 0) rhs = sea_node_idom(rhs);
    }
}

U16 sea_node_loop_depth(SeaNode *n) {
    Assert(sea_node_is_cfg(n));
    // Loop depth is cfg->vint execpt for proj cuz proj has vint as its index
    if (n->vint != 0 && n->kind != SeaNodeKind_Proj) return n->vint;
    switch (n->kind) {
        case SeaNodeKind_Region: {
            U16 d = sea_node_loop_depth(n->inputs[1]);
            n->vint = d;
            return d;
        } break;
        case SeaNodeKind_Loop: {
            // Loop depth is set to the entry's loop depth + 1
            U16 d = sea_node_loop_depth(n->inputs[1]) + 1;
            n->vint = d;


        } break;
        case SeaNodeKind_Proj: {
            // vint is already proj index
            SeaNode *idom = n->inputs[0];
            if (idom->kind == SeaNodeKind_If) {
                SeaNode *if_idom = idom->inputs[0];
                if (if_idom->kind == SeaNodeKind_Loop) {
                    // if we the exit projection of a loopnode we decrement the loopdepth
                    if (n->vint == 0) return sea_node_loop_depth(idom) - 1;
                }
            }

            return sea_node_loop_depth(idom);

        } break;
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop: {
            U16 d = 1;
            n->vint = d;
            return d;
        } break;
        default: {
            U16 d = sea_node_loop_depth(n->inputs[0]);
            n->vint = d;
            return d;
        } break;
    }

}


B32 node_is_pinned(SeaNode *n) {
    switch (n->kind) {
        case SeaNodeKind_Proj:
        case SeaNodeKind_Copy:
        case SeaNodeKind_Phi: return 1;
    }
    return sea_node_is_cfg(n);
}

// Skip iteration if a backedge
 // private static boolean isForwardsEdge(Node use, Node def) {
 //     return use != null && def != null &&
 //         !(use.nIns()>2 && use.in(2)==def && (use instanceof LoopNode || (use instanceof PhiNode phi && phi.region() instanceof LoopNode)));
 // }
 //


 B32 is_forward_edge(SeaNode *u, SeaNode *d) {

    return d && u && !(u->inputlen > 2 && (u->inputs[2] == d) && (
        u->kind == SeaNodeKind_Loop ||
        (u->kind == SeaNodeKind_Phi  && u->inputs[0]->kind == SeaNodeKind_Loop)));
 }

void sea_node_list_push_tail(SeaNodeList *l, SeaNodeNode *n) {
    SLLQueuePush(l->head, l->tail, n);
    l->count += 1;
}

void sea_node_list_push_head(SeaNodeList *l, SeaNodeNode *n) {
    SLLQueuePushFront(l->head, l->tail, n);
    l->count += 1;
}

void rpo_cfg(Arena *arena, BitArray *visit, SeaNodeList *l, SeaNode *n) {
    if (!sea_node_is_cfg(n) || bits_get(visit, n->nid)) return;
    bits_set(visit, n->nid);

    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        rpo_cfg(arena, visit, l, user);
    }

    SeaNodeNode *nn = push_item(arena, SeaNodeNode);
    nn->node = n;
    sea_node_list_push_head(l, nn);
}

SeaNode *cfg_zero(SeaNode *n) {
    if (n->kind == SeaNodeKind_Start) return 0;
    SeaNode *cfg0 = n->inputs[0];
    if (n->kind == SeaNodeKind_Proj && cfg0->kind != SeaNodeKind_Start)
        return cfg0->inputs[0];
    return cfg0;
}

B32 node_is_blockhead(SeaNode *cfg) {
    Assert(sea_node_is_cfg(cfg));
    switch (cfg->kind) {
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop:
        case SeaNodeKind_Proj:
            return 1;
    }
    return 0;
}


void schedule_early(SeaFunctionGraph *fn, BitArray *visit, SeaNode *n) {
    if (!n || bits_get(visit, n->nid)) return;
    bits_set(visit, n->nid);

    for EachIndex(i, n->inputlen) {
        SeaNode *in = n->inputs[i];
        if (in && !node_is_pinned(in))
        schedule_early(fn, visit, in);
    }

    if (!node_is_pinned(n)) {
        SeaNode *early = fn->start;

        for EachIndexFrom(i, 1, n->inputlen) {
            SeaNode *in = n->inputs[i];
            if (in) {
                SeaNode *cfg0 = cfg_zero(in);
                U16 d0 = sea_node_idepth(early);
                U16 d1 = sea_node_idepth(cfg0);
                if (d1 > d0) early = cfg0;
            }

        }

        Assert(early);
        sea_node_set_input(fn, n, early, 0);
    }
}

SeaNode *use_block(SeaNode *n, SeaNode *user, SeaNode **late) {
    if (user->kind != SeaNodeKind_Phi) {
        return late[user->nid];
    }

    SeaNode *cfg_found = 0;
    for EachIndexFrom(i, 1, user->inputlen) {
        SeaNode *in = user->inputs[i];
        if (in == n) {
            if (!cfg_found) {
                SeaNode *region = user->inputs[0];
                Assert(i < region->inputlen);
                cfg_found = region->inputs[i];
            } else {
                NotImplemented;
            }
        }
    }

    Assert(cfg_found);
    return cfg_found;
}

B32 better(SeaNode *lca, SeaNode *best) {
    B32 better_loop_depth = sea_node_loop_depth(lca) < sea_node_loop_depth(best);
    B32 is_if = best->kind == SeaNodeKind_If;
    B32 greater_idepth = sea_node_idepth(lca) > sea_node_idepth(best);
    return better_loop_depth || is_if || greater_idepth;
}

void schedule_late(
    SeaFunctionGraph *fn,
    SeaNode **late,
    SeaNode **nodes,
    SeaNode *n
) {
    if (late[n->nid]) return;

    if (sea_node_is_cfg(n))
        late[n->nid] = node_is_blockhead(n) ? n : n->inputs[0];
    if (n->kind == SeaNodeKind_Phi)
        late[n->nid] = n->inputs[0];

    // TODO walk memory things

    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        if (is_forward_edge(user, n))
            schedule_late(fn, late, nodes, user);
    }

    if (node_is_pinned(n)) return;

    SeaNode *early = n->inputs[0];
    Assert(early);
    SeaNode *lca = 0;
    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        SeaNode *blk = use_block(n, user, late);
        lca = sea_node_lca(blk, lca);
    }

    SeaNode *best = lca;
    for (;lca != sea_node_idom(early); lca = sea_node_idom(lca)) {
        if (better(lca, best)) {
            best = lca;
        }
    }

    Assert(best->kind != SeaNodeKind_If);
    nodes[n->nid] = n;
    late[n->nid] = best;
}


void validiate_scheduling(BitArray *visit, SeaNode *n) {
    if (bits_get(visit, n->nid)) return;
    bits_set(visit, n->nid);

    if (!sea_node_is_cfg(n)) {
        Assert(n->inputs[0]);
    }

    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        validiate_scheduling(visit, user);
    }
}

void sea_global_code_motion(SeaFunctionGraph *fn) {

    // Early Scheduling
    {
        Temp scratch = scratch_begin(0, 0);
        BitArray visit = bits_alloc(scratch.arena, fn->nidcap);

        SeaNodeList l = { 0 };
        rpo_cfg(scratch.arena, &visit, &l, fn->start);
        for EachNode(nn, SeaNodeNode, l.head) {
            SeaNode *cfg = nn->node;
            sea_node_loop_depth(cfg);
            for EachIndex(i, cfg->inputlen) {
                SeaNode *in = cfg->inputs[i];
                schedule_early(fn, &visit, in);

                if (
                    cfg->kind == SeaNodeKind_Loop ||
                    cfg->kind == SeaNodeKind_Region
                ) {
                    for EachNode(user_node, SeaUser, cfg->users) {
                        SeaNode *user = sea_user_val(user_node);
                        if (user->kind == SeaNodeKind_Phi)
                            schedule_early(fn, &visit, user);
                    }
                }
            }
        }
        scratch_end(scratch);
    }


    // Schedule late
    {
        Temp scratch = scratch_begin(0, 0);
        SeaNode **late  = push_array(scratch.arena, SeaNode*, fn->nidcap);
        SeaNode **nodes = push_array(scratch.arena, SeaNode*, fn->nidcap);
        schedule_late(fn, late, nodes, fn->start);
        for EachIndex(i, fn->nidcap) {
            SeaNode *n = nodes[i];
            SeaNode *block = late[i];
            if (n) {
                Assert(block);
                sea_node_set_input(fn, n, block, 0);
            }
        }

        scratch_end(scratch);
    }
    #define SEA_DEBUG 0
    #if SEA_DEBUG
    {
        Temp scratch = scratch_begin(0, 0);
        BitArray visit = bits_alloc(scratch.arena, fn->nidcap);

        validiate_scheduling(&visit, fn->start);
        scratch_end(scratch);
    }
    #endif
}
