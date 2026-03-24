#include "sea_internal.h"


typedef struct LiveRange LiveRange;
struct LiveRange {
    SeaNode *node;
    U32 def;
    U32 lastuse;
    U8 colour;
};

typedef struct LiveRangeCell LiveRangeCell;
struct LiveRangeCell {
    SeaNode *key;
    LiveRangeCell *next;
    LiveRange range;
};


typedef struct LiveRanges LiveRanges;
struct LiveRanges {
    LiveRange **ranges;
    U64 rangelen;
    U64 rangecap;
    LiveRangeCell **cells;
    U64 cellcap;
    Arena *arena;
};


static U64 live_ranges_next_prime(U64 n) {
    // heuristic: start search from 2x node count
    n = n * 2 + 1;
    for (;;) {
        B32 prime = 1;
        for (U64 i = 2; i * i <= n; i++) {
            if (n % i == 0) { prime = 0; break; }
        }
        if (prime) return n;
        n++;
    }
}

static inline B32 live_range_overlaps(LiveRange *a, LiveRange *b) {
    return a->def <= b->lastuse && b->def <= a->lastuse;
}

LiveRanges live_ranges_init(Arena *arena, U64 node_count) {
    U64 cellcap  = live_ranges_next_prime(node_count);
    U64 rangecap = node_count;
    LiveRanges lr = (LiveRanges){
        .arena    = arena,
        .cells    = push_array(arena, LiveRangeCell*, cellcap),
        .cellcap  = cellcap,
        .ranges   = push_array(arena, LiveRange*, rangecap),
        .rangecap = rangecap,
    };
    return lr;
}

LiveRange *live_ranges_insert(LiveRanges *lr, SeaNode *key, U64 def) {
    U64 hash = sea_node_hash(key);
    LiveRangeCell **slot = &lr->cells[hash % lr->cellcap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return &(*slot)->range;
        }
        slot = &(*slot)->next;
    }
    LiveRangeCell *cell = push_item(lr->arena, LiveRangeCell);
    cell->key  = key;
    cell->next = NULL;
    *slot = cell;

    Assert(lr->rangelen < lr->rangecap);
    lr->ranges[lr->rangelen++] = &cell->range;

    LiveRange *rng = &cell->range;
    rng->def = def;
    rng->lastuse = def;
    rng->node = key;

    return rng;
}

LiveRange *live_ranges_lookup(LiveRanges *lr, SeaNode *key) {
    U64 hash = sea_node_hash(key);
    LiveRangeCell **slot = &lr->cells[hash % lr->cellcap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return &(*slot)->range;
        }
        slot = &(*slot)->next;
    }
    return NULL;
}


B32 node_needs_reg(SeaNode *n) {
    switch (n->kind) {
        // control flow - no output value
        case X64Node_Ret:
        case X64Node_Jmp:
        case X64Node_Cmp:
        case X64Node_CmpI:
        // cloned nodes that carry no value
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop:
        case SeaNodeKind_Loop:
        case SeaNodeKind_Region:
        case SeaNodeKind_If:
            return 0;

        case SeaNodeKind_Proj:
            return !sea_node_is_cfg(n);

        // produce a value
        case X64Node_AddI:
        case X64Node_Add:
        case X64Node_SubI:
        case X64Node_Sub:
        case X64Node_MulI:
        case X64Node_Mul:
        case X64Node_Set:
        case SeaNodeKind_Copy:
        case SeaNodeKind_Phi:
            return 1;

        // div is special - value comes out via proj
        case X64Node_Div:
            return 0;

        default: { Trap(); } break;
    }
}

void build_intervals(SeaFunctionGraph *fn, LiveRanges *lr) {
    for EachIndex(i, fn->schedlen) {
        SeaNode *n = fn->sched[i];
        if (node_needs_reg(n)) {
            live_ranges_insert(lr, n, i);
        }

        for EachIndexFrom(idx, 1, n->inputlen) {
            SeaNode *in = n->inputs[idx];
            if (node_needs_reg(in)) {
                LiveRange *rng = live_ranges_lookup(lr, in);
                rng->lastuse = Max(rng->lastuse, i);
            }
        }
    }
}


void reg_alloc(SeaFunctionGraph *fn, LiveRanges *lr) {
    Temp scratch = scratch_begin(&lr->arena, 1);

    RegMask pool = rmask_u64(0xFF);
    LiveRange **active = push_array(scratch.arena, LiveRange *, lr->rangelen);
    U64 activelen = 0;

    for EachIndex(i, lr->rangelen) {
        // expire old intervals
        LiveRange *curr = lr->ranges[i];
        for EachIndex(j, activelen) {
            LiveRange *rng = active[j];
            if (rng->lastuse <= curr->def) {
                // expired - free its register
                pool = rmask_set(pool, rng->colour);
                // remove from active by swapping with last
                active[j] = active[--activelen];
                j--; // recheck this slot
            }
        }

        // try to find optimial reg
        RegMask r = mach.rmask_out(curr->node);
        r = rmask_and(r, pool);
        if (rmask_empty_p(r)) {
            NotImplemented;
        }

        RegMask out = r;
        for EachNode(user_node, SeaUser, curr->node->users) {
            SeaNode *user = sea_user_val(user_node);
            U16 slot = sea_user_slot(user_node);
            RegMask in = mach.rmask_in(user, slot);
            out = rmask_and(out, in);
        }

        S32 colour = rmask_empty_p(out) ? rmask_get_first_empty(r) : rmask_get_first_empty(out);
        Assert(colour >= 0); // -1 means no reg found
        curr->colour = (U8)colour;
        pool = rmask_unset(pool, colour);
        active[activelen++] = curr;

    }

    scratch_end(scratch);
}

void linear_reg_alloc(SeaFunctionGraph *fn) {
    LiveRanges lr = live_ranges_init(fn->arena, fn->node_count);
    build_intervals(fn, &lr);
    reg_alloc(fn, &lr);
    LiveRanges *reg_data = push_item(fn->arena, LiveRanges);
    MemoryCopyStruct(reg_data, &lr);
    fn->reg_data = reg_data;
}

void sea_reg_alloc(SeaFunctionGraph *fn) {
    linear_reg_alloc(fn);
}

S32 sea_get_reg_colour(SeaFunctionGraph *fn, SeaNode *n) {
    LiveRanges *lr  = fn->reg_data;
    S32 result = -1;

    LiveRange *rng = live_ranges_lookup(lr, n);
    if (rng) result = rng->colour;

    return result;
}
