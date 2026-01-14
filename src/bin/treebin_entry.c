#include <core.h>
#include <stdio.h>

#include <front/tokenizer.h>
#include <front/front.h>


S32 entry_point(String *args, U64 arg_count) {
    // tokenizer_init();
    frontend_init();
    printf("%.*s\n", str_arg(s));
    Arena *src_arena = arena_init(MEGABYTE(4), ArenaFlag_Chainable | ArenaFlag_LargePage);
    printf("\nwhat: %p\n", src_arena);
    SrcFile srcfile = (SrcFile){.name = args[1], .src = os_read_entire_file(src_arena, args[1])};

    // printf("%*.s\n=============================\n%*.s\n", str_arg(srcfile.name), str_arg(srcfile.src));
    printf("%*.s\n", str_arg(srcfile.name));


    return 0;
}
