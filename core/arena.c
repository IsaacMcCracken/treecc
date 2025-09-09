#include <core.h>

Arena *arena_init(U64 cap) {
    cap = mem_align_forward(cap, PAGE_SIZE);

    void *backing = os_reserve(cap);
    Arena *arena = (Arena*)backing;
    if (!arena) return 0;
    // we gotta commit at least one page

    os_commit(arena, PAGE_SIZE);

    *arena = (Arena){
        .pos = sizeof(Arena),
        .cmt = PAGE_SIZE,
        .cap = cap,
    };

    return arena;
}

void arena_clear(Arena *arena) {
    arena->pos = sizeof(Arena);
}

void arena_deinit(Arena *arena) {
    os_release(arena, arena->cap);
}

void *arena_get_current(Arena *arena) {
    char *result = ((char*)arena) + arena->pos;
    return result;
}

void *arena_push_no_zero_(Arena *arena, U64 size, U64 align) {
    U64 start = mem_align_forward(arena->pos, align);
    U64 end = start + size;

    if (end > arena->cmt) {
        U64 new_cmt = mem_align_forward(end, PAGE_SIZE);
        os_commit((void*)arena, new_cmt);
        arena->cmt = new_cmt;
    }

    char *result = ((char*)arena) + start;
    arena->pos = end;
    return result;
}

void *arena_push_(Arena *arena, U64 size, U64 align) {
    void *result = arena_push_no_zero_(arena, size, align);
    mem_zero(result, size);
    return result;
}


TempArena temp_arena_begin(Arena *arena) {
    return (TempArena){
        .pos = arena->pos,
        .arena = arena,
    };
}

void temp_arena_end(TempArena temp) {
    temp.arena->pos = temp.pos;
}
