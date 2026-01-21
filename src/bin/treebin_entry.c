#include <stdio.h>

#include <front/tokenizer.h>
#include <front/front.h>


internal void entry_point(CmdLine *cmd_line) {
    // frontend_init();
    printf("WTF");
    for EachNode(cmd, String8Node, cmd_line->inputs.first) {
            printf("%.*s", str8_varg(cmd->string));
            Parser p = parser_begin(cmd->string);
    }

    // Parser p = parser_begin(args[1]);

    frontend_deinit();
}
