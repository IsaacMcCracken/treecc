#include <core.h>

Arena *arena_init_(ArenaParams args) {
    ArenaFlags flags = args.flags;

    U64 cmt_size = PAGE_SIZE;
    if (flags & ArenaFlag_LargePage) cmt_size = LARGE_PAGE_SIZE;

    U64 cap = mem_align_forward(args.cap, cmt_size);

    // we get the large pages
    void *backing = 0;
    if (flags & ArenaFlag_LargePage) {
        backing = os_reserve_large(cap);
    } else {
        backing = os_reserve(cap);
    }


    Arena *arena = (Arena*)backing;
    if (!arena) return 0;
    // we gotta commit at least one page
    if (flags & ArenaFlag_LargePage) {
        os_commit_large(backing, cmt_size);
    } else {
        os_commit(backing, cmt_size);
    }


    *arena = (Arena){
        .pos = sizeof(Arena),
        .cmt = PAGE_SIZE,
        .cap = cap,
        .flags = args.flags,
        .curr = arena,
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
    // get the current arena
    Arena *curr = arena->curr; // probably_self

    U64 start = mem_align_forward(curr->pos, align);
    U64 end = start + size;

    if (end > curr->cap) {
        if (arena->flags & ArenaFlag_Chainable) {
            Arena *new = 0;
            if (arena->free) {
                new = arena->free;
                arena->free = new->prev;
            } else {
                new = arena_init(DEFAULT_ARENA_SIZE);
                new->prev = curr;
            }
            new->prev = curr;
            curr = new;
            arena->curr = curr;
        } else {
            // TODO: ERROR OUT OF MEMORY
            return 0;
        }
    }

    if (end > curr->cmt) {
        U64 new_cmt = 0;
        if (arena->flags & ArenaFlag_LargePage) {
            new_cmt = mem_align_forward(end, LARGE_PAGE_SIZE);
            os_commit_large((void*)curr, new_cmt);
        } else {
            new_cmt = mem_align_forward(end, PAGE_SIZE);
            os_commit((void*)curr, new_cmt);
        }
        curr->cmt = new_cmt;
    }


    char *result = ((char*)curr) + start;
    curr->pos = end;
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
