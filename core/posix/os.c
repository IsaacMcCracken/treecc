#include <core.h>
#include <sys/mman.h>


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

void os_protect(void *ptr, U64 size, OSMemoryFlags flags) {
    int flags = posix_memory_flags(flags);
    mprotect(ptr, size, flags);
}

void os_release(void *ptr, U64 size) {
    munmap(ptr, size);
}
