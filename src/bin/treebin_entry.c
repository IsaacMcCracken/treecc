#include <core.h>
#include <stdio.h>

#include <front/tokenizer.h>
#include <front/front.h>


S32 entry_point(String *args, U64 arg_count) {
    frontend_init();

    Parser p = parser_begin(args[1]);

    frontend_deinit();
    return 0;
}
