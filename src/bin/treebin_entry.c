#include <core.h>
#include <stdio.h>

#include <front/tokenizer.h>
#include <front/front.h>


S32 entry_point(String *args, U64 arg_count) {
    // tokenizer_init();
    frontend_init();

    Parser p = parser_init(args[1]);
    printf("\n%.*s\n%d\n", str_arg(f->text), f->tok_count);

    frontend_deinit();
    return 0;
}
