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
}

void thread_context_select(ThreadContext *ctx) {
    thread_context = ctx;
}

ThreadContext *thread_context_selected(void) {
    return thread_context;
}
