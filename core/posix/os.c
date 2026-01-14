#include <core.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>


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
    OSSystemInfo sys_info;
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
    os_posix_state.entity_arena = arena_init(MEGABYTE(64));
    if (os_posix_state.entity_arena == 0) return 0;

    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count == -1) {
        return 0;
    }

    os_posix_state.sys_info.cpu_count = (U32)cpu_count;

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

void *os_reserve_large(U64 size) {
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (result == MAP_FAILED)
        return 0;
    return result;
}

B32 os_commit(void *ptr, U64 size) {
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return 1;
}

B32 os_commit_large(void *ptr, U64 size) {
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
    return (Buffer){ .ptr = (S8*)buf, .len = file_size };
}

void *os_posix_thread_entry(void *ptr) {
    PosixEntity *e = (PosixEntity*)ptr;
    FnThreadEntry fn = e->thread.fn;
    void *params = e->thread.ptr;
    void *result = thread_entry_point(fn, params);
    return result;
}

Thread os_thread_launch(FnThreadEntry fn, void *ptr) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_Thread);
    e->thread.fn = fn;
    e->thread.ptr = ptr;

    int result = pthread_create(&e->thread.handle, 0, os_posix_thread_entry, e);
    if (result == -1) {
        os_posix_entity_free(e);
        e = 0;
    }

    return (Thread){(U64)e};
}

// TODO what the fuck does ryan mean by this
void os_thread_detach(Thread t) {
    if (t.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)t.id[0];
    os_posix_entity_free(e);
}


B32 os_thread_join(Thread thread, U64 endt_us) {
    // TODO is_mem_zero(ptr, size)
    if (!thread.id[0]) {
        return 0;
    }
    PosixEntity *e = (PosixEntity*)thread.id[0];
    int success = pthread_join(e->thread.handle, 0);
    B32 result = success == 0;
    os_posix_entity_free(e);
    return result;
}

Mutex os_mutex_alloc(void) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_Mutex);
    int result = pthread_mutex_init(&e->mutex, 0);
    if (result == -1) {
        os_posix_entity_free(e);
        e = 0;
    }

    return (Mutex){(U64)e};
}

void os_mutex_free(Mutex m) {
    if (m.id[0] == 0) {
        return;
    }

    PosixEntity *e = (PosixEntity*)m.id[0];
    pthread_mutex_destroy(&e->mutex);
    os_posix_entity_free(e);
}

void os_mutex_lock(Mutex m) {
    if (m.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)m.id[0];
    pthread_mutex_lock(&e->mutex);
}

void os_mutex_unlock(Mutex m) {
    if (m.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)m.id[0];
    pthread_mutex_unlock(&e->mutex);
}

RWMutex os_rwmutex_alloc(void) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_RWMutex);
    int result = pthread_rwlock_init(&e->rwmutex, 0);
    if (result == -1) {
        os_posix_entity_free(e);
        e = 0;
    }
    return (RWMutex){(U64)e};
}

void os_rwmutex_free(RWMutex m) {
  if (m.id[0] == 0) { return; }
  PosixEntity *e = (PosixEntity*)m.id[0];
  pthread_rwlock_destroy(&e->rwmutex);
  os_posix_entity_free(e);
}

void os_rwmutex_lock(RWMutex m, RWMutexMode mode) {
    if (m.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)m.id[0];
    if (mode) {
        pthread_rwlock_wrlock(&e->rwmutex);
    } else {
        pthread_rwlock_rdlock(&e->rwmutex);
    }
}


void os_rwmutex_unlock(RWMutex m) {
    if (m.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)m.id[0];
    pthread_rwlock_unlock(&e->rwmutex);
}

Barrier os_barrier_alloc(U64 count) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_Barrier);
    pthread_barrier_init(&e->barrier, 0, count);
    return (Barrier){(U64)e};
}

void os_barrier_free(Barrier b) {
    if (b.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)b.id[0];
    pthread_barrier_destroy(&e->barrier);
    os_posix_entity_free(e);
}


void os_barrier_wait(Barrier b) {
    if (b.id[0] == 0) return;
    PosixEntity *e = (PosixEntity*)b.id[0];
    pthread_barrier_wait(&e->barrier);
}

 CondVar os_cond_var_alloc(void) {
    PosixEntity *e = os_posix_entity_alloc(PosixEntityKind_CondVar);
    int result = pthread_cond_init(&e->cv.cond, 0);
    if (result == -1) {
        os_posix_entity_free(e);
        return (CondVar){ 0 };
    }

    result = pthread_mutex_init(&e->cv.mutex, 0);
    if (result == -1) {
        pthread_cond_destroy(&e->cv.cond);
        os_posix_entity_free(e);
        e = 0;
    }

    return (CondVar){(U64)e};
}

void os_cond_var_free(CondVar cv) {
    PosixEntity *e = (PosixEntity*)cv.id[0];
    pthread_cond_destroy(&e->cv.cond);
    pthread_mutex_destroy(&e->cv.mutex);
    os_posix_entity_free(e);
}

void os_cond_var_wait(CondVar cv, Mutex m, U64 endt_us) {

}


FileInfo os_get_file_info(Arena *arena, String path) {
    FileInfo info = {0};

    static thread_static char resolved[PATH_MAX];

    TempArena temp = temp_arena_begin(arena);
    char *cpath = string_to_cstring(arena, path);
    // Resolve to absolute path
    if (!realpath(cpath, resolved)) {
        return info; // not found or invalid
    }
    temp_arena_end(temp);

    struct stat st;
    if (stat(resolved, &st) != 0) {
        return info;
    }

    // Copy fullpath into arena
    U64 len = (U64)strlen(resolved);
    S8 *dst = arena_push_array(arena, S8, len + 1);
    mem_cpy(dst, resolved, len + 1);

    info.fullpath.ptr   = dst;
    info.fullpath.len   = len;
    info.size           = (U64)st.st_size;
    info.is_dir         = S_ISDIR(st.st_mode);

#if defined(__APPLE__)
    info.modified.nsec = (U64)st.st_mtimespec.tv_sec * 1000000000ull +
                    (U64)st.st_mtimespec.tv_nsec;
#else
    info.modified.nsec = (U64)st.st_mtim.tv_sec * 1000000000ull +
                    (U64)st.st_mtim.tv_nsec;
#endif

    return info;
}

OSHandle os_file_open(OSFileFlags flags, String path) {
    TempArena scratch = scratch_begin(0, 0);

    char *cpath = string_to_cstring(scratch.arena, path);

    int posix_flags = 0;

    if (flags & OSFileFlag_Read && flags & OSFileFlag_Write) posix_flags |= O_RDWR;
    else if (flags & OSFileFlag_Read) posix_flags |= O_RDONLY;
    else if (flags & OSFileFlag_Write) posix_flags |= O_WRONLY;
    if (flags & (OSFileFlag_Write | OSFileFlag_Append)) posix_flags |= O_CREAT;
    if (flags & OSFileFlag_Create) posix_flags |= O_CREAT;
    posix_flags |= O_CLOEXEC;
    int fd = open(cpath, posix_flags, 0755); // find out what 0755 does?
    scratch_end(scratch);

    OSHandle h = fd;
    if (fd == -1) h = 0;
    return h;
}

void os_file_close(OSHandle file) {
    if (file == 0) return;
    int fd = (int)file;
    close(fd);
}

// U64 os_file_read(OSHandle file,)

OSSystemInfo os_get_system_info(void) {
    return os_posix_state.sys_info;
}
