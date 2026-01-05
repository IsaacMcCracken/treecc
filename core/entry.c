#if defined (CORE_PLATFORM_WINDOWS)
#include "windows/entry.c"
#else
#include "posix/entry.c"
#endif


void *thread_entry_point(FnThreadEntry fn, void *params) {
    ThreadContext *ctx = thread_context_alloc();
    thread_context_select(ctx);
    void *result = fn(params);
    thread_context_free(ctx);
    return result;
}
