
#include "stdio.h"
#include "soup/soup.h"
#include "frontend/parser.h"

char *src = "return 1 + 3 * 4;";

int main(int argc, char **argv) {
    Arena *arena = arena_init(1<<20);
    U32 tokcount = 0;
    TreeToken *tokens = tree_tokenize(arena, &tokcount, (Byte*)src, strlen(src));
    TreeParser p = {
        .arena = arena,
        .tokens = tokens,
        .tokencount = tokcount,
        .src = (Byte*)src,
    };

    print_tokens(tokens, tokcount, (Byte*)src);
}
