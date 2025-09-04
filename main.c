
#include "stdio.h"
// #include "soup/soup.h"
#include "frontend/parser.h"

char *src = "return 1 + 2 * 3;";


TreeParser tree_parse(char *src) {
    Arena *arena = arena_init(1<<20);
    U32 tokcount = 0;
    TreeToken *tokens = tree_tokenize(arena, &tokcount, (Byte*)src, strlen(src));
    SoupFunction fn = (SoupFunction){
        .arena = arena,
        .map = soup_map_init(64<<12, 101),
    };
    TreeParser p = {
        .arena = arena,
        .tokens = tokens,
        .tokencount = tokcount,
        .src = (Byte*)src,
        .fn = fn,
    };


    SoupNode *ret = tree_parse_stmt(&p);
    printf("Constfold ret %ld\n", ret->inputs[1]->vint);
    return p;
}

int main(int argc, char **argv) {
    tree_parse(src);
}
