#include "sea_internal.h"


SeaWorkList sea_worklist_init(Arena *arena, U64 itemcap, U64 hashcap) {
    SeaWorkList wl = {0};
    wl.items = push_array(arena, SeaNode*, itemcap);
    wl.itemcap = itemcap;
    wl.hashcap = hashcap;
    wl.hashset = push_array(arena, U32*, hashcap);
    return wl;
}

B32 sea_worklist_contains(SeaWorkList *wl, SeaNode *node) {
    U64 slot = node->gvn % wl->hashcap;
    U32 *curr = wl->hashset[slot];
    while (curr) {
        if (*curr == node->gvn) return 1;
        curr++;
    }
    return 0;
}

void sea_worklist_push(Arena *arena, SeaWorkList *wl, SeaNode *node) {
    if (sea_worklist_contains(wl, node)) return;
    // add to items
    if (wl->itemlen >= wl->itemcap) {
        U64 newcap = wl->itemcap * 2;
        SeaNode **new_items = push_array(arena, SeaNode*, newcap);
        MemoryCopyTyped(new_items, wl->items, wl->itemlen);
        wl->items = new_items;
        wl->itemcap = newcap;
    }
    wl->items[wl->itemlen++] = node;
    // add to hashset
    U64 slot = node->gvn % wl->hashcap;
    U32 *entry = push_item(arena, U32);
    *entry = node->gvn;
    // store as linked list via pointer arithmetic - just use a flat array per bucket
    wl->hashset[slot] = entry;
}

SeaNode *sea_worklist_pop(SeaWorkList *wl) {
    if (wl->itemlen == 0) return 0;
    return wl->items[--wl->itemlen];
}

B32 sea_worklist_empty(SeaWorkList *wl) {
    return wl->itemlen == 0;
}

void sea_worklist_clear(SeaWorkList *wl) {
    wl->itemlen = 0;
    MemoryZeroTyped(wl->hashset, wl->hashcap);
}
