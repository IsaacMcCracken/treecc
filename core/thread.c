#include <core.h>

thread_static ThreadContext *thread_context;

ThreadPool *thread_pool_alloc(U64 max_tasks) {
    U64 arena_size = sizeof(Arena) + sizeof(ThreadPool) + sizeof(ThreadPoolWorker) * max_tasks;
    Arena *arena = arena_init(arena_size);
    ThreadPool *pool = arena_push(arena, ThreadPool);
    pool->arena = arena;


    return pool;
}

void thread_pool_free(ThreadPool *pool) {
    arena_deinit(pool->arena);
}

ThreadContext *thread_context_alloc(void) {
    Arena *arena = arena_init(MEGABYTE(64));
    ThreadContext *ctx = arena_push(arena, ThreadContext);
    ctx->arenas[0] = arena;
    ctx->arenas[1] = arena_init(MEGABYTE(64));
    return ctx;
}

void thread_context_free(ThreadContext *ctx) {
    arena_deinit(ctx->arenas[1]);
    arena_deinit(ctx->arenas[0]);
    if (thread_context == ctx) thread_context = 0;
}

void thread_context_select(ThreadContext *ctx) {
    thread_context = ctx;
}

ThreadContext *thread_context_selected(void) {
    return thread_context;
}

Arena *thread_context_get_scratch(Arena **conflicts, U64 count) {
    ThreadContext *ctx = thread_context_selected();
    Arena *result = 0;
    Arena **arena_ptr = ctx->arenas;
    for(U64 i = 0; i < 2; i += 1, arena_ptr += 1) {
        Arena **conflict_ptr = conflicts;
        B32 has_conflict = 0;
        for(U64 j = 0; j < count; j += 1, conflict_ptr += 1) {
            if(*arena_ptr == *conflict_ptr) {
                has_conflict = 1;
                break;
            }
        }
        if(!has_conflict) {
            result = *arena_ptr;
            break;
        }
    }
    return result;
}
