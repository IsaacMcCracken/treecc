#include "sea_internal.h"


typedef struct SeaIdomCell SeaIdomCell;
struct SeaIdomCell {
    SeaNode *key;
    SeaBlock *value;
    SeaIdomCell *next;
};

typedef struct SeaIdomMap SeaIdomMap;
struct SeaIdomMap {
    SeaIdomCell **cells;
    U64 cap;
    Arena *arena;
};

SeaIdomMap sea_idom_map_init(Arena *arena, U64 cap) {
    return (SeaIdomMap){
        .cells = push_array(arena, SeaIdomCell*, cap),
        .cap   = cap,
        .arena = arena,
    };
}

void sea_idom_map_insert(SeaIdomMap *map, SeaNode *key, SeaBlock *value) {
    U64 hash = sea_node_hash(key);
    SeaIdomCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            (*slot)->value = value;
            return;
        }
        slot = &(*slot)->next;
    }
    SeaIdomCell *cell = push_item(map->arena, SeaIdomCell);
    cell->key   = key;
    cell->value = value;
    cell->next  = NULL;
    *slot = cell;
}

SeaBlock *sea_idom_map_lookup(SeaIdomMap *map, SeaNode *key) {
    U64 hash = sea_node_hash(key);
    SeaIdomCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return (*slot)->value;
        }
        slot = &(*slot)->next;
    }
    return NULL;
}

B32 node_starts_bb(SeaNode *n) {
    switch (n->kind) {
        case SeaNodeKind_Start:
        case SeaNodeKind_Region:
        case SeaNodeKind_Loop:
            return 1;
        case SeaNodeKind_Proj:
            return sea_node_is_cfg(n);
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

void _pretty_print_node(SeaNode *n, U16 *ids) {
    Temp scratch = scratch_begin(0, 0);
    printf("    ");
    switch (n->kind) {
        case X64Node_Jmp:
        case SeaNodeKind_If: {
            int cond_id = ids[(n->inputs[1])->nid];
            String8 flabel, tlabel;
            int fid, tid;

            for EachNode(user_node, SeaUser, n->users) {
                SeaNode *user = sea_user_val(user_node);
                if (sea_node_is_cfg(user) && user->kind == SeaNodeKind_Proj) {
                    switch (user->vint) {
                        case 0: {
                            flabel = sea_node_label(scratch.arena, user);
                            fid = ids[user->nid];
                        } break;
                        case 1: {
                            tlabel = sea_node_label(scratch.arena, user);
                            tid = ids[user->nid];
                        } break;
                        default: {Trap();}
                    }
                }
            }

            printf("if v%d jump BB%d_%.*s else jump BB%d_%.*s", cond_id, tid, str8_varg(tlabel), fid, str8_varg(flabel));

        } break;
        default: {
            String8 label = sea_node_label(scratch.arena, n);
            if (n->kind != SeaNodeKind_Return || n->kind == X64Node_Ret) {
                int id = ids[n->nid];
                printf("v%d ", id);
            }

            printf("%.*s ", str8_varg(label));
            for EachIndexFrom(i, 1, n->inputlen) {
                SeaNode *in = n->inputs[i];
                int id = ids[in->nid];
                printf("v%d ", id);
            }
        }
    }

    scratch_end(scratch);
    printf("\n");
}

void _pretty_print(SeaFunctionGraph *fn, SeaBlock *bb, U16 *ids) {
    U16 bbid = ids[bb->begin->nid];

    String8 bblabel = sea_node_label(0, bb->begin);
    printf("BB%d_%.*s:\n", (int)bbid, str8_varg(bblabel));
    for EachIndex(i, bb->nodelen) {
        SeaNode *n = bb->nodes[i];
        _pretty_print_node(n, ids);
    }

    for EachNode(_bb, SeaBlock, bb->children.head) {
        _pretty_print(fn, _bb, ids);
    }
}

void _pretty_print_ids(SeaFunctionGraph *fn, SeaBlock *bb, U16 *ids, U16 *id, U16 *bbid)  {
    U16 _bbid = *bbid;
    ids[bb->begin->nid] = _bbid;
    *bbid += 1;

    for EachIndex(i, bb->nodelen) {
        SeaNode *n = bb->nodes[i];
        ids[n->nid] = *id;
        *id += 1;
    }

    for EachNode(_bb, SeaBlock, bb->children.head) {
        _pretty_print_ids(fn, _bb, ids, id, bbid);
    }
}


void pretty_print(SeaFunctionGraph *fn) {
    U16 id = 0; U16 bbid = 0;
    Temp scratch = scratch_begin(0, 0);
    U16 *ids = push_array(scratch.arena, U16, fn->nidcap);

    _pretty_print_ids(fn, fn->domtree, ids, &id, &bbid);
    _pretty_print(fn, fn->domtree, ids);

    scratch_end(scratch);
}

SeaBlock *build_dom_tree(SeaFunctionGraph *fn, SeaIdomMap *map, BitArray *visit, SeaNode *n) {
    if (!n || bits_get(visit, n->nid) || !sea_node_is_cfg(n)) return 0; // missing n->nid
    bits_set(visit, n->nid);

    SeaBlock *out = 0;

    if (node_starts_bb(n)) {
        SeaBlock *bb = sea_alloc_item(fn, SeaBlock);
        bb->begin = n;
        sea_idom_map_insert(map, n, bb);

        out = bb;

        SeaNode *idom = sea_node_idom(n);
        while (idom && !node_starts_bb(idom)) {
            idom = sea_node_idom(idom);
        }
        if (idom) {
            build_dom_tree(fn, map, visit, idom);
            SeaBlock *bbdom = sea_idom_map_lookup(map, idom);
            Assert(bbdom);  // missing semicolon
            DLLPushBack(bbdom->children.head, bbdom->children.tail, bb); // missing semicolon
        } else {
            fn->domtree = bb; // set root
        }
    }

    for EachNode(user_node, SeaUser, n->users) {
        SeaNode *user = sea_user_val(user_node);
        build_dom_tree(fn, map, visit, user); // was passing n instead of user
    }

    return out;
}

B32 is_back_edge(SeaNode *src, SeaNode *dst) {
    return dst->inputlen > 2
        && dst->inputs[2] == src
        && (dst->kind == SeaNodeKind_Loop ||
           (dst->kind == SeaNodeKind_Phi && dst->inputs[0]->kind == SeaNodeKind_Loop));
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

void sched_dom_tree(SeaFunctionGraph *fn, SeaBlock *bb, U16 *dep_data) {
    sched_bb(fn, bb, dep_data);

    for EachNode(_bb, SeaBlock, bb->children.head) {
        sched_dom_tree(fn, _bb, dep_data);
    }
}

void sea_local_schedule(SeaFunctionGraph *fn) {
    // get blocks
    {
        Temp scratch = scratch_begin(0,0);
        BitArray visit = bits_alloc(scratch.arena, fn->nidcap);
        SeaIdomMap map = sea_idom_map_init(scratch.arena, 401);

        SeaBlock *root = build_dom_tree(fn, &map, &visit, fn->start);
        Assert(root);

        fn->domtree = root;

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

        sched_dom_tree(fn, fn->domtree, dep_data);

        scratch_end(scratch);
    }

    // print
    {
        pretty_print(fn);
    }
}
