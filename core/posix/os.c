#include <core.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
