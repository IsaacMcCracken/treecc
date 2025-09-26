#include <core.h>
#include <windows.h>




OSHandle os_file_open(OSFileFlags flags, String path) {
    OSHandle file = 0;


    return file;    
}

DWORD windows_memory_flags(OSMemoryFlags flags) {
    int read  = flags & OSMemoryFlags_Read;
    int write = flags & OSMemoryFlags_Write;
    int exec  = flags & OSMemoryFlags_Exec;

    if (exec) {
        if (read && write) return PAGE_EXECUTE_READWRITE;
        if (read)          return PAGE_EXECUTE_READ;
        if (write)         return PAGE_EXECUTE_READWRITE; // closest match
        return PAGE_EXECUTE;
    } else {
        if (read && write) return PAGE_READWRITE;
        if (read)          return PAGE_READONLY;
        if (write)         return PAGE_READWRITE; // no PAGE_WRITEONLY
        return PAGE_NOACCESS;
    }
}



void os_file_close(OSHandle file) {
    // probably check for zero
    HANDLE h = (HANDLE)file;
    BOOL r = CloseHandle(h);
}

void *os_reserve(U64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

void os_protect(void *ptr, U64 size, OSMemoryFlags flags) {
    DWORD protectionFlags = windows_memory_flags(flags);
    DWORD oldProtect;
    VirtualProtect(ptr, size, protectionFlags, &oldProtect);
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

