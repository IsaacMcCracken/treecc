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

#ifdef CORE_MSVC
#define thread_static __declspec(thread)

// TODO check computer archetecture
// TODO add atomics for MSVC
#include <intrin.h>
#else
#define thread_static __thread
#  define atomic_u64_eval(x)                 __atomic_load_n(x, __ATOMIC_SEQ_CST)
#  define atomic_u64_inc_eval(x)             (__atomic_fetch_add((volatile U64 *)(x), 1, __ATOMIC_SEQ_CST) + 1)
#  define atomic_u64_dec_eval(x)             (__atomic_fetch_sub((volatile U64 *)(x), 1, __ATOMIC_SEQ_CST) - 1)
#  define atomic_u64_eval_assign(x,c)        __atomic_exchange_n(x, c, __ATOMIC_SEQ_CST)
#  define atomic_u64_add_eval(x,c)           (__atomic_fetch_add((volatile U64 *)(x), c, __ATOMIC_SEQ_CST) + (c))
#  define atomic_u64_eval_cond_assign(x,k,c) ({ U64 _new = (c); __atomic_compare_exchange_n((volatile U64 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
#  define atomic_u32_eval(x)                 __atomic_load_n(x, __ATOMIC_SEQ_CST)
#  define atomic_u32_inc_eval(x)             (__atomic_fetch_add((volatile U32 *)(x), 1, __ATOMIC_SEQ_CST) + 1)
#  define atomic_u32_add_eval(x,c)           (__atomic_fetch_add((volatile U32 *)(x), c, __ATOMIC_SEQ_CST) + (c))
#  define atomic_u32_eval_assign(x,c)        __atomic_exchange_n(x, c, __ATOMIC_SEQ_CST)
#  define atomic_u32_eval_cond_assign(x,k,c) ({ U32 _new = (c); __atomic_compare_exchange_n((volatile U32 *)(x),&_new,(k),0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST); _new; })
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
#define GIGABYTE(X) ((U64)(X)) << 30ULL

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

#define DEFAULT_ARENA_SIZE MEGABYTE(64)

typedef U8 ArenaFlags;
enum {
    ArenaFlag_Chainable = 0x1,
    ArenaFlag_LargePage = 0x2,
};

typedef struct {
    U64 cap;
    ArenaFlags flags;
} ArenaParams;

typedef struct Arena Arena;
struct Arena {
    U64 pos;
    U64 cmt;
    U64 cap;
    Arena *prev;
    Arena *curr;
    Arena *free;
    ArenaFlags flags;
    U64 _reserved;
};

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
    char *ptr;
    U64 len;
} Buffer;

typedef Buffer String;

typedef struct HashEntry HashEntry;
struct HashEntry {
    HashEntry *next_hash;
    HashEntry *next_iter;
};

typedef U64 *(*FnHash)(void *data, U64 size);

typedef struct {
    Arena *arena;
    HashEntry **buf;
    U64 cap;
    U64 entry_count;
    U64 entry_size;
    FnHash *hash;
} HashMap;

typedef struct {
    Arena *arena;
    char *base;
    U64 len;
} BufferBuilder;


typedef struct {
    S64 nsec;
} Time;

typedef struct {
    U64 id[1];
} Thread;

typedef struct {
    Arena *arenas[2];
} ThreadContext;

typedef struct ThreadPoolWorker ThreadPoolWorker;
struct ThreadPoolWorker {
    ThreadPoolWorker *next;
    Thread thread;
};

typedef struct {
    ThreadPoolWorker *head;
    ThreadPoolWorker *tail;
    U64 count;
} ThreadPoolWorkerList;

typedef struct {
    Arena *arena; // allocation for the pool
    ThreadPoolWorkerList workers;
    ThreadPoolWorker *free_list;
} ThreadPool;

typedef struct {
    U64 id[1];
} Mutex;

typedef struct {
    U64 id[1];
} RWMutex;

typedef U8 RWMutexMode;
enum {
  RWMutexMode_Read = 0,
  RWMutexMode_Write = 1,
};

typedef struct {
    U64 id[1];
} Barrier;

typedef struct {
    U64 id[1];
} CondVar;

typedef void *(*FnThreadEntry)(void *params);

typedef struct {
    String fullpath; // probably allocated
    String name; // points to fullpath
    U64 size;
    Time modified;
    B32 is_dir;
} FileInfo;

typedef UIntPtr OSHandle;

typedef struct {
    U32 cpu_count;

} OSSystemInfo;

typedef U16 OSFileFlags;
enum {
    OSFileFlag_Read     = 0x1,
    OSFileFlag_Write    = 0x2,
    OSFileFlag_Append   = 0x4,
    OSFileFlag_Create   = 0x8,
    // maybe more
};

typedef U8 OSMemoryFlags;
enum {
    OSMemoryFlags_Read  = 0x1,
    OSMemoryFlags_Write = 0x2,
    OSMemoryFlags_Exec  = 0x4,
};

S32 entry_point(String *args, U64 arg_count);
void *thread_entry_point(FnThreadEntry fn, void *params);

// S32 core_init(void);

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



OSSystemInfo os_get_system_info(void);
OSHandle os_file_open(OSFileFlags flags, String path);
void os_file_close(OSHandle file);
void *os_reserve(U64 size);
void *os_reserve_large(U64 size);
B32 os_commit(void *ptr, U64 size);
B32 os_commit_large(void *ptr, U64 size);
void os_decommit(void *ptr, U64 size);
void os_release(void *ptr, U64 size);
void os_protect(void *ptr, U64 size, OSMemoryFlags flags);
Buffer os_read_entire_file(Arena *arena, String path);

Mutex os_mutex_alloc(void);
void os_mutex_free(Mutex m);
void os_mutex_lock(Mutex m);
void os_mutex_unlock(Mutex m);

RWMutex os_rwmutex_alloc(void);
void os_rwmutex_free(RWMutex m);
void os_rwmutex_lock(RWMutex m, RWMutexMode mode);
void os_rwmutex_unlock(RWMutex m);

Barrier os_barrier_alloc(U64 count);
void os_barrier_free(Barrier b);
void os_barrier_wait(Barrier b);

U64 mem_align_backward(U64 x, U64 align);
U64 mem_align_forward(U64 x, U64 align);


Arena *arena_init_(ArenaParams args);
#define arena_init(...) arena_init_((ArenaParams){__VA_ARGS__})
void arena_clear(Arena *arena);
void *arena_push_(Arena *arena, U64 size, U64 align);
void *arena_get_current(Arena *arena);
void arena_deinit(Arena *arena);
TempArena temp_arena_begin(Arena *arena);
void temp_arena_end(TempArena temp);

Pool pool_init(Arena *arena, U64 chunk_align, U64 chunk_size);
void *pool_alloc(Pool *pool);

U64 string_hash(String s);
S64 string_parse_int(String a);
char *string_to_cstring(Arena *arena, String s);
String cstring_to_string(char *str);
S32 string_cmp(String a, String b);
String string_alloc(Arena *arena, const char *str);
String string_cpy(Arena *arena, String source);
String string_concat(Arena *arena, int count, ...);
#define str_lit(str) (String){ str, sizeof(str) - 1 }
#define str_arg(str) (int)str.len, str.ptr

//**************************************//
//          Thread Functions            //
//**************************************//

ThreadContext *thread_context_alloc(void);
void thread_context_select(ThreadContext *ctx);
void thread_context_free(ThreadContext *ctx);
ThreadContext *thread_context_selected(void);
Arena *thread_context_get_scratch(Arena **conflicts, U64 count);
#define scratch_begin(conflicts, count) temp_arena_begin(thread_context_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_arena_end(scratch)

Thread os_thread_launch(FnThreadEntry fn, void *ptr);
B32 os_thread_join(Thread thread, U64 endt_us);
void os_thread_detach(Thread t);

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


HashMap *hashmap_init_(FnHash hash, U64 cap, U64 entry_size);
#define hashmap_init(value, hash, cap) hashmap_init_(hash, cap, mem_align_forward(sizeof(value) + sizeof(HashEntry), mem_alignof(HashEntry)))


#endif // CORE_H
