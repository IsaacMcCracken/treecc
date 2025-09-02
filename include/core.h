#pragma once

#include <stdint.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#define PAGE_SIZE (1<<12)
#define LARGE_PAGE_SIZE (1<<16)

typedef uint8_t Byte;

typedef uint32_t B32;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int64_t S64;

typedef struct {
    U64 pos;
    U64 cmt;
    U64 cap;
    U64 _un;
} Arena;

typedef struct {
    U64 pos;
    Arena *arena;
} TempArena;

typedef struct {
    char *str;
    U64 len;
} String;



void *arena_push_(Arena *arena, U64 size, U64 align);

#ifndef CORE_MACROS
#define CORE_MACROS
#define StrLit(str) (String){str, sizeof(str)-1}

#define mem_set(s, c, n) memset(s, c, n)
#define mem_zero(s, n) memset(s, 0, n)
#define mem_alignof(x) __alignof__(x)
#define mem_cmp(a, b, n) memcmp(a, b, n)
#define mem_cpy(d, s, n) memcpy(d, s, n)
#define mem_cpy_array(D, S, T, N) memcpy(D, S, sizeof(T)*(N))

#define arena_push_no_zero(arena, T) (T *)arena_push_no_zero_(arena, sizeof(T), mem_alignof(T))
#define arena_push(arena, T) (T *)arena_push_(arena, sizeof(T), mem_alignof(T))
#define arena_push_array(arena, T, N) (T *)arena_push_(arena, sizeof(T)*(N), mem_alignof(T))
#endif // CORE_MACROS

#if defined (CORE_IMPLEMENTATION)



U64 mem_align_backward(U64 x, U64 align) {
    return x & ~(align -1);
}

U64 mem_align_forward(U64 x, U64 align) {
    return mem_align_backward(x + (align - 1), align);
}

void *os_reserve(U64 size) {
    void * result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) return 0;
    return result;
}


B32 os_commit(void* ptr, U64 size) {
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return 1;
}

void os_decommit(void *ptr, U64 size) {
    madvise(ptr, size, MADV_DONTNEED);
    mprotect(ptr, size, PROT_NONE);
}

void os_release(void *ptr, U64 size) {
    munmap(ptr, size);
}


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
    return result;
}

void *arena_push_(Arena *arena, U64 size, U64 align) {
    void *result = arena_push_no_zero_(arena, size, align);
    mem_zero(result, size);
    return result;
}





#endif
