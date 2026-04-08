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

typedef struct SeaPatch SeaPatch;
struct SeaPatch {
    U64 loc;
    SeaNode *desired;
};

#define MAX_PATCHES 128

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

S64 sea_pos_map_lookup(SeaPosMap *map, SeaNode *key) {
    U64 hash = sea_node_hash(key);
    SeaPosCell **slot = &map->cells[hash % map->cap];
    while (*slot) {
        if (sea_node_equal((*slot)->key, key)) {
            return (*slot)->value;
        }
        slot = &(*slot)->next;
    }
    return -1;
}


int cmp_block(const void *a, const void *b) {
    SeaBlock *bba = *(SeaBlock **)a;
    SeaBlock *bbb = *(SeaBlock **)b;
    if (bba->begin->kind == SeaNodeKind_Proj) return -1 - bba->begin->vint;
    if (bbb->begin->kind == SeaNodeKind_Proj) return 1 + bbb->begin->vint;
    return 0;
}

void encode_block(
    SeaEmitter *e,
    SeaFunctionGraph *fn,
    SeaPosMap *map,
    SeaBlock *bb,
    SeaPatch *patches,
    U16 *patchlen
) {
    U64 block_start = e->len;
    sea_pos_map_insert(map, bb->begin, block_start);

    for EachIndex(i, bb->nodelen) {
        SeaNode *n = bb->nodes[i];
        S64 loc = mach.encode(e, fn, n);
        if (n->kind == X64Node_Jmp) {
            Assert(loc != -1);
            // find false branch
            SeaNode *desired = 0;
            for EachNode(user_node, SeaUser, n->users) {
                SeaNode *user = sea_user_val(user_node);
                if (sea_node_is_cfg(user) &&
                    user->kind == SeaNodeKind_Proj &&
                    user->vint == 0
                ) {
                    desired = user;
                    break;
                }
            }
            patches[*patchlen] = (SeaPatch){.loc = loc, .desired = desired};
            *patchlen += 1;
            Assert(*patchlen <= MAX_PATCHES);
        }
    }

    for EachNode(_bb, SeaBlock, bb->children.head) {
        encode_block(e, fn, map, _bb, patches, patchlen);
    }

}

void sea_encode(SeaModule *m, SeaFunctionGraph *fn) {
    SeaEmitter *e = &m->emit;
    U64 start = e->len;
    SeaSymbolEntry *entry = sea_lookup_symbol(m, fn->proto.name);
    entry->pos_in_section = start;

    Temp scratch = scratch_begin(0, 0);
    SeaPosMap map = sea_pos_map_init(scratch.arena, 401);
    SeaPatch *patches = push_array(scratch.arena, SeaPatch, MAX_PATCHES);
    U16 patchlen = 0;

    encode_block(e, fn, &map, fn->domtree, patches, &patchlen);

    for EachIndex(i, patchlen) {
        SeaPatch p = patches[i];
        S64 desired_loc = sea_pos_map_lookup(&map, p.desired);
        Assert(desired_loc != -1);
        S64 patch_loc = p.loc;
        S64 next_loc = patch_loc + 4;
        S32 offset = desired_loc - next_loc;
        emitter_write_s32(e, patch_loc, offset);
    }


    scratch_end(scratch);

    for EachIndexFrom(i, start, e->len) {
        printf("0x%02X ", e->code[i]);
    }

    printf("\n");

}
