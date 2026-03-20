#include "sea_internal.h"

extern SeaMach mach;

typedef struct SeaNodePairCell SeaNodePairCell;
struct SeaNodePairCell {
    SeaNode *key;
    SeaNode *value;
    SeaNodePairCell *next;
};

typedef struct SeaNodePairMap SeaNodePairMap;
struct SeaNodePairMap {
    SeaNodePairCell **cells;
    U64 cap;
    Arena *arena;
};

SeaNodePairMap sea_pair_map_init(Arena *arena, U64 cap) {
    SeaNodePairCell **cells = push_array(arena, SeaNodePairCell*, cap);
    SeaNodePairMap map = (SeaNodePairMap){
        .arena = arena,
        .cells = cells,
        .cap = cap,
    };
    return map;
}

void sea_pair_map_insert(SeaNodePairMap *map, SeaNode *key, SeaNode *value) {
    U64 hash = sea_node_hash(key);
    SeaNodePairCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            (*slot)->value = value;
            return;
        }
        slot = &(*slot)->next;
    }
    SeaNodePairCell *cell = push_item(map->arena, SeaNodePairCell);
    cell->key = key;
    cell->value = value;
    cell->next = 0;
    *slot = cell;
}

SeaNode *sea_pair_map_lookup(SeaNodePairMap *map, SeaNode *key) {
    U64 hash = sea_node_hash(key);
    SeaNodePairCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return (*slot)->value;
        }
        slot = &(*slot)->next;
    }
    return 0;
}




SeaNode *instr_select_walk(SeaFunctionGraph *fn, SeaNodePairMap *map, SeaNode *n) {
    if (!n) return 0;
    SeaNode *x = sea_pair_map_lookup(map, n);
    if (x) return x;

    if (sea_node_is_mach(n)) {
        for EachIndex(i, n->inputlen) {
            SeaNode *old_input = n->inputs[i];
            SeaNode *new_input = instr_select_walk(fn, map, old_input);
            sea_node_set_input_raw(n, new_input, i);
        }
        return n;
    }

    x = mach.select(fn, n);
    sea_pair_map_insert(map, n, x);

    for EachIndex(i, x->inputlen) {
        SeaNode *old_input = x->inputs[i];
        SeaNode *new_input = instr_select_walk(fn, map, old_input);
        sea_node_set_input_raw(x, new_input, i);
    }

    // todo mach.post_select


    sea_node_remove_all_users_raw(n);
    return x;
}

SeaNode *instr_select_walk_for_outputs_and_return_start(
    SeaFunctionGraph *fn,
    BitArray *visit,
    SeaNode *n
) {
    if (bits_get(visit, n->nid)) return 0;
    bits_set(visit, n->nid);

    SeaNode *s = 0;
    if (n->kind == SeaNodeKind_Start) s = n;

    for EachIndex(i, n->inputlen) {
        SeaNode *in = n->inputs[i];
        if (in) {
            sea_node_append_user(fn, in, n, i);
            SeaNode *o = instr_select_walk_for_outputs_and_return_start(fn, visit, in);
            if (o) {
                Assert(!s);
                s = o;
            }
        }
    }


    return s;
}

void sea_instruction_selection(SeaFunctionGraph *fn) {

    SeaNode *mach_stop = 0;
    SeaNode *mach_start = 0;
    {
        Temp scratch = scratch_begin(0, 0);
        // TODO size heuristic
        SeaNodePairMap map = sea_pair_map_init(scratch.arena, 101);

        mach_stop = instr_select_walk(fn, &map, fn->stop);

        scratch_end(scratch);
    }

    {
        Temp scratch = scratch_begin(0, 0);
        // TODO size heuristic
        BitArray visit = bits_alloc(scratch.arena, fn->nidcap);
        mach_start = instr_select_walk_for_outputs_and_return_start(fn, &visit, mach_stop);

        scratch_end(scratch);
    }


    Assert(mach_stop && mach_start);

    fn->start = mach_start;
    fn->stop = mach_stop;
}
