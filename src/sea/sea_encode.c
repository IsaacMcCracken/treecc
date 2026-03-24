#include "sea_internal.h"

typedef struct SeaPosCell SeaPosCell;
struct SeaPosCell {
    SeaNode *key;
    U64 value;
    SeaPosCell *next;
};

typedef struct SeaPosMap SeaPosMap;
struct SeaPosMap {
    SeaPosCell **cells;
    U64 cap;
    Arena *arena;
};

SeaPosMap sea_pos_map_init(Arena *arena, U64 cap) {
    return (SeaPosMap){
        .cells = push_array(arena, SeaPosCell*, cap),
        .cap   = cap,
        .arena = arena,
    };
}

void sea_pos_map_insert(SeaPosMap *map, SeaNode *key, U64 value) {
    U64 hash = sea_node_hash(key);
    SeaPosCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            (*slot)->value = value;
            return;
        }
        slot = &(*slot)->next;
    }
    SeaPosCell *cell = push_item(map->arena, SeaPosCell);
    cell->key   = key;
    cell->value = value;
    cell->next  = NULL;
    *slot = cell;
}

U64 sea_pos_map_lookup(SeaPosMap *map, SeaNode *key, U64 default_val) {
    U64 hash = sea_node_hash(key);
    SeaPosCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return (*slot)->value;
        }
        slot = &(*slot)->next;
    }
    return default_val;
}


int cmp_block(const void *a, const void *b) {
    SeaBlock *bba = *(SeaBlock **)a;
    SeaBlock *bbb = *(SeaBlock **)b;
    if (bba->begin->kind == SeaNodeKind_Proj) return -1 - bba->begin->vint;
    if (bbb->begin->kind == SeaNodeKind_Proj) return 1 + bbb->begin->vint;
    return 0;
}

void encode_block(SeaEmitter *e, SeaFunctionGraph *fn, SeaPosMap *map, SeaBlock *bb, SeaBlock **sibs, U64 idx) {
    U64 block_start = e->len;
    sea_pos_map_insert(map, bb->begin, block_start);

    for EachIndex(i, bb->nodelen) {
        SeaNode *n = bb->nodes[i];
        mach.encode(e, fn, n);
    }

    // sort children
    Temp scratch = scratch_begin(&map->arena, 1);
    SeaBlock **children = arena_pos_ptr(scratch.arena);
    U64 child_count = 0;
    for EachNode(_bb, SeaBlock, bb->children.head) {
        SeaBlock **kid = push_item(scratch.arena, SeaBlock*);
        *kid = _bb;
        child_count += 1;
    }
    quick_sort(children, child_count, sizeof(SeaBlock*), cmp_block);

    // use children here before ending scratch
    for EachIndex(i, child_count) {
        encode_block(e, fn, map, children[i], children, i);
    }

    scratch_end(scratch);
}

void sea_encode(SeaModule *m, SeaFunctionGraph *fn) {
    SeaEmitter *e = &m->emit;
    U64 start = e->len;
    SeaSymbolEntry *entry = sea_lookup_symbol(m, fn->proto.name);
    entry->pos_in_section = start;

    Temp scratch = scratch_begin(0, 0);
    SeaPosMap map = sea_pos_map_init(scratch.arena, 401);

    encode_block(e, fn, &map, fn->domtree, 0, 0);

    scratch_end(scratch);

    for EachIndexFrom(i, start, e->len) {
        printf("0x%02X ", e->code[i]);
    }

    printf("\n");

}
