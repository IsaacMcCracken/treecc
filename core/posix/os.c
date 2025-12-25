#include <core.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

typedef U8 PosixEntityKind;
enum {
    PosixEntityKind_Invalid,
    PosixEntityKind_Mutex,
    PosixEntityKind_RWMutex,
    PosixEntityKind_Barrier,
    PosixEntityKind_CondVar,
    PosixEntityKind_Thread,
};

typedef struct PosixEntity PosixEntity;
struct PosixEntity {
    PosixEntity *next;
    PosixEntityKind kind;
    union {
        pthread_mutex_t mutex;
        pthread_rwlock_t rwmutex;
        pthread_barrier_t barrier;
        struct {
            pthread_cond_t cond;
            pthread_mutex_t mutex;
        } cv;
        struct {
            pthread_t handle;
            FnThreadEntry fn;
            void *ptr;
        } thread;
    };
};


struct {
    pthread_mutex_t entity_mutex;
    Arena *entity_arena;
    PosixEntity *free_entity;
} os_posix_state = { 0 };

PosixEntity *os_posix_entity_alloc(PosixEntityKind kind) {
    PosixEntity *e = 0;
    {
        pthread_mutex_lock(&os_posix_state.entity_mutex);

        if (os_posix_state.free_entity) {
            e = os_posix_state.free_entity;
            os_posix_state.free_entity = os_posix_state.free_entity->next;
            mem_zero_item(e, PosixEntity);
        } else {
            e = arena_push(os_posix_state.entity_arena, PosixEntity);
        }
        pthread_mutex_unlock(&os_posix_state.entity_mutex);
    }

    e->kind = kind;
    return e;
}

void os_posix_entity_free(PosixEntity *e) {
    pthread_mutex_lock(&os_posix_state.entity_mutex);

    e->next = os_posix_state.free_entity;
    os_posix_state.free_entity = e;

    pthread_mutex_unlock(&os_posix_state.entity_mutex);
}


S32 os_posix_core_lib_init(void) {
    os_posix_state.entity_arena = arena_init(GIGABYTE(1));
    if (os_posix_state.entity_arena == 0) return 0;

    return 1;
}

int posix_memory_flags(OSMemoryFlags flags) {
    int out = 0;
    if (flags & OSMemoryFlags_Read) {
        out |= PROT_READ;
    }

    if (flags & OSMemoryFlags_Write) {
        out |= PROT_WRITE;
    }

    if (flags & OSMemoryFlags_Exec) {
        out |= PROT_EXEC;
    }

    return out;
}

void *os_reserve(U64 size) {
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
        return 0;
    return result;
}

B32 os_commit(void *ptr, U64 size) {
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return 1;
}

void os_decommit(void *ptr, U64 size) {
    madvise(ptr, size, MADV_DONTNEED);
    mprotect(ptr, size, PROT_NONE);
}

void os_protect(void *ptr, U64 size, OSMemoryFlags flags) {
    int pflags = posix_memory_flags(flags);
    mprotect(ptr, size, pflags);
}

void os_release(void *ptr, U64 size) {
    munmap(ptr, size);
}

Buffer os_read_entire_file(Arena *arena, String path) {

    TempArena temp = temp_arena_begin(arena);
    char *cpath = string_to_cstring(arena, path);

    int file = open(cpath, O_RDONLY);
    temp_arena_end(temp);
    if (file == -1) {
        /*handle error*/
    }
    struct stat file_data;
    if (fstat(file, &file_data) == -1) {
        /*handle error*/
    }
    size_t file_size = file_data.st_size;
    char *buf = arena_push_array(arena, char, file_size);
    size_t amt_read = 0;
    while (amt_read != file_size) {
        amt_read += read(file, buf + amt_read, file_size - amt_read);
    }
    close(file);
    return (Buffer){ .str = buf, .len = file_size };
}

Thread os_thread_launch(FnThreadEntry fn, void *ptr) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_Thread);
    e->thread.fn = fn;
    e->thread.ptr = ptr;

    // TODO launch thread
    // int result = pthread_create(&e->thread.handle, 0, )
    return (Thread){(U64)e};
}
