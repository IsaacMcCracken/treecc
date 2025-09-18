#include <core.h>

U64 mem_align_backward(U64 x, U64 align) {
    return x & ~(align -1);
}

U64 mem_align_forward(U64 x, U64 align) {
    return mem_align_backward(x + (align - 1), align);
}

Pool pool_init(Arena *arena, U64 chunk_align, U64 chunk_size) {
    chunk_align = mem_align_forward(chunk_align, 8);
    chunk_size = mem_align_forward(chunk_size, 16);
    return (Pool){
        .arena = arena,
        .chunk_align = chunk_align,
        .chunk_size = chunk_size,
    };
}

void *pool_alloc(Pool *pool) {
    if (pool->freelist) {
        void *next = *((void**)pool->freelist);
        void *chunk = pool->freelist;
        pool->freelist = next;
        return chunk;
    } else {
        return arena_push_(pool->arena, pool->chunk_size, pool->chunk_align);
    }
}

void pool_free(Pool *pool, void *ptr) {
    // security check if was allocated on the pool
    if ((ptr < (void*)(pool->arena + sizeof(Arena))) || ptr > (void*)(pool->arena + pool->arena->cmt)) {
        // gotta figure what to do here

    }

    void **next = (void **)ptr;
    *next = pool->freelist;
    pool->freelist = ptr;
}
