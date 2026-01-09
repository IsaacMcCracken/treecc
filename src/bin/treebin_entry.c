#include <core.h>
#include <stdio.h>




S32 entry_point(String *args, U64 arg_count) {
    Arena *arena = arena_init(MEGABYTE(64), ArenaFlag_Chainable | ArenaFlag_LargePage);

    printf("Hello Mother\n");

    return 0;
}
