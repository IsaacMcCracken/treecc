
#include <core.h>

#define MAX_ARGS 128
S32 os_posix_core_lib_init(void);


int main(int argc, char **argv) {
    assert(argc <= MAX_ARGS);
    String args[MAX_ARGS];
    U64 arg_count = (U64)argc;

    for (int i = 0; i < argc; i += 1) {
        args[i] = cstring_to_string(argv[i]);
    }

    os_posix_core_lib_init();

    ThreadContext *ctx = thread_context_alloc();
    thread_context_select(ctx);
    int result = (int)entry_point(args, arg_count);
    thread_context_free(ctx);
    return result;
}
