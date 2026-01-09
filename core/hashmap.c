#include <core.h>

HashMap *hashmap_init_(FnHash hash, U64 cap, U64 entry_size) {
    U64 arena_size = sizeof(void*) * cap + (2 * cap * entry_size)/3 + sizeof(HashMap);
    Arena *arena = arena_init(arena_size, ArenaFlag_Chainable, ArenaFlag_LargePage);
    HashMap *map = arena_push(arena, HashMap);
    map->hash = hash;
    map->cap = cap;
    map->entry_size = entry_size;
    map->buf = arena_push_array(arena, void*, cap);
    return map;
}
