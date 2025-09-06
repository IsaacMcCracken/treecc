#include <core.h>
#include <windows.h>

void *os_reserve(U64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}


B32 os_commit(void* ptr, U64 size) {
    B32 result = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0;
    return result;
}

void os_decommit(void *ptr, U64 size) {
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

void os_release(void *ptr, U64 size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

