
#include "stdio.h"
// #include "soup/soup.h"
#include "frontend/parser.h"

char *src = "int fn() {return 1 + 2;}";

TreeParser tree_parse(char *src) {
    Arena *arena = arena_init(2<<20);
    U32 tokcount = 0;
    TreeToken *tokens = tree_tokenize(arena, &tokcount, (Byte*)src, strlen(src));
    Arena *map_arena = arena_init(64<<12);

    U64 type_map_cap = 4093;
    TreeTypeMap types = {
        .arena = arena,
        .cells = arena_push_array(arena, TreeTypeMapCell*, type_map_cap),
        .cap = type_map_cap,
    };

    SoupFunction fn = (SoupFunction){
        .arena = arena,
        .map = soup_map_init(map_arena, 101),
    };
    TreeParser p = {
        .arena = arena,
        .tokens = tokens,
        .tokencount = tokcount,
        .types = types,
        .src = (Byte*)src,
        .fn = fn,
    };


    TreeDecl *decl = tree_parse_decl(&p);
    TreeFnDecl *fndecl = (TreeFnDecl*)decl;
    return p;
}

int main(int argc, char **argv) {

    tree_tokenizer_init();
    tree_parse(src);
}
