/*
 * Core Library
 * Designed to get around the hardships of C and have all the things you
 * you need to get started on a project right away
 *
 * Copyright Isaac McCracken, Thomas Gordon
 *
 */

#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

// Platform Things
#ifdef _WIN32
#define CORE_PLATFORM_WINDOWS
#else
#define CORE_PLATFORM_POSIX
#endif

#ifdef _MSC_VER
#define CORE_MSVC
#pragma warning(disable : 4477)
#endif

typedef uint8_t Byte;
typedef uintptr_t UIntPtr;

typedef uint32_t B32;

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

#define PAGE_SIZE (1 << 12)
#define LARGE_PAGE_SIZE (1 << 16)

#define KILOBYTE(X) (X) << 10
#define MEGABYTE(X) (X) << 20
#define GIGABYTE(X) (U64)(X) << 30

#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFFUL
#define U64_MAX 0xFFFFFFFFFFFFFFFFULL

#define S8_MIN (-0x80)
#define S8_MAX 0x7F
#define S16_MIN (-0x8000)
#define S16_MAX 0x7FFF
#define S32_MIN (-0x7FFFFFFF - 1) // guaranteed int literal
#define S32_MAX 0x7FFFFFFF
#define S64_MIN (-0x7FFFFFFFFFFFFFFFLL - 1) // avoids literal overflow
#define S64_MAX 0x7FFFFFFFFFFFFFFFLL

#define TIME_NANOSECOND     1LL
#define TIME_MICROSECOND    1000LL
#define TIME_MILLISECOND    1000000LL
#define TIME_SECOND         1000000000LL
#define TIME_MINUTE         60000000000LL

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

typedef String Buffer;

typedef struct {
    Arena *arena;
    Byte *base;
    U64 len;
} BufferBuilder;

typedef struct {
    S64 nsec;
} Time;

typedef struct {
    U64 id[1];
} Thread;

typedef void (*FnThreadEntry)(void *params);

typedef struct {
    String fullpath; // probably allocated
    String name; // points to fullpath
    U64 size;
    B32 is_dir;
} FileInfo;

typedef UIntPtr OSHandle;

typedef U16 OSFileFlags;
enum {
    OSFileFlag_Read,
    OSFileFlag_Write,
    OSFileFlag_Append,
    OSFileFlag_Create,
    // maybe more
};

typedef U8 OSMemoryFlags;
enum {
    OSMemoryFlags_Read = 0x1,
    OSMemoryFlags_Write = 0x2,
    OSMemoryFlags_Exec = 0x4,
};

S32 core_init(void);

BufferBuilder builder_init(Arena *arena);
String builder_to_string(BufferBuilder *bb);
void builder_write_string(BufferBuilder *bb, String string);
void builder_write_number(BufferBuilder *bb, U64 num, const U8 base);
void builder_write_signed_dec(BufferBuilder *bb, S64 num);
#define builder_write_unsigned_dec(bb, num) builder_write_number(bb, num, 10)
#define builder_write_oct(bb, num) builder_write_number(bb, num, 8)
#define builder_write_hex(bb, num) builder_write_number(bb, num, 16)
#define builder_write_bin(bb, num) builder_write_number(bb, num, 2)
void builder_write_float(BufferBuilder *bb, float num, const U8 precision);

OSHandle os_file_open(OSFileFlags flags, String path);
void os_file_close(OSHandle file);
void *os_reserve(U64 size);
B32 os_commit(void *ptr, U64 size);
void os_decommit(void *ptr, U64 size);
void os_release(void *ptr, U64 size);
void os_protect(void *ptr, U64 size, OSMemoryFlags flags);
Buffer os_read_entire_file(Arena *arena, String path);

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

//**************************************//
//          Thread Functions            //
//**************************************//

Thread thread_launch(FnThreadEntry entry, void *params);

#define str_lit(str) (String){ str, sizeof(str) - 1 }

#define mem_set(s, c, n) memset(s, c, n)
#define mem_zero(s, n) memset(s, 0, n)
#define mem_zero_item(S, T) memset(S, 0, sizeof(T))
#ifdef CORE_MSVC
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
