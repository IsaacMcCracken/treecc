#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define PAGE_SIZE (1 << 12)
#define LARGE_PAGE_SIZE (1 << 16)

typedef uint8_t Byte;

typedef uint32_t B32;


typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
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
    Arena *arena;
    U64 chunk_align;
    U64 chunk_size;
    void *freelist;
} Pool;

typedef struct {
    char *str;
    U64 len;
} String;

typedef U8 OSMemoryFlags;
enum {
    OSMemoryFlags_Read = 0x1,
    OSMemoryFlags_Write = 0x2,
    OSMemoryFlags_Exec = 0x4,
}

void *os_reserve(U64 size);
B32 os_commit(void *ptr, U64 size);
void os_decommit(void *ptr, U64 size);
void os_release(void *ptr, U64 size);
void os_protect(void *ptr, U64 size, OSMemoryFlags flags)

U64 mem_align_backward(U64 x, U64 align);
U64 mem_align_forward(U64 x, U64 align);

Arena *arena_init(U64 init);
void arena_clear(Arena *arena);
void *arena_push_(Arena *arena, U64 size, U64 align);
void *arena_get_current(Arena *arena);
void arena_deinit(Arena *arena);
TempArena temp_arena_begin(Arena *arena);
void temp_arena_end(TempArena temp);

Pool pool_init(Arena *arena, U64 chunk_align, U64 chunk_size);
void *pool_alloc(Pool *pool);

S64 string_parse_int(String a);
char *string_to_cstring(Arena *arena, String s);
S32 string_cmp(String a, String b);
String string_alloc(Arena *arena, const char *str);
String string_cpy(Arena *arena, String source);
String string_concat(Arena *arena, int count, ...);

#define StrLit(str) (String){ str, sizeof(str) - 1 }

#define mem_set(s, c, n) memset(s, c, n)
#define mem_zero(s, n) memset(s, 0, n)
#define mem_zero_item(S, T) memset(S, 0, sizeof(T))
#ifdef _WIN32
#define mem_alignof(x) __alignof(x)
#else
#define mem_alignof(x) __alignof__(x)
#endif
#define mem_cmp(a, b, n) memcmp(a, b, n)
#define mem_cpy(d, s, n) memcpy(d, s, n)
#define mem_cpy_array(D, S, T, N) memcpy(D, S, sizeof(T) * (N))
#define mem_cpy_item(D, S, T) memcpy(D, S, sizeof(T))

#define arena_push_no_zero(arena, T) \
    (T *)arena_push_no_zero_(arena, sizeof(T), mem_alignof(T))
#define arena_push(arena, T) (T *)arena_push_(arena, sizeof(T), mem_alignof(T))
#define arena_push_array(arena, T, N) \
    (T *)arena_push_(arena, sizeof(T) * (N), mem_alignof(T))

#endif // CORE_H
