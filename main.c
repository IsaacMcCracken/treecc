
#include "stdio.h"
// #include "soup/soup.h"
#include "frontend/parser.h"
//temp
#include "soup/x64/x64.c"

char *src = "return 1 + 2;";

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


    SoupNode *ret = tree_parse_stmt(&p);
    printf("Constfold ret %ld %hd\n", ret->inputs[1]->vint, ret->inputs[1]->kind);
    return p;
}

int main(int argc, char **argv) {
    Arena *arena = arena_init(1<<20);
    X64Emiter e = x64_emiter_init(arena);
    x64_encode_add(&e, X64GPRegister_RAX, X64GPRegister_R8);
    x64_encode_near_jmp(&e, 7);
    x64_encode_ret(&e);

    for (int i = 0; i < e.len; i ++) {
        printf("0x%02X, ", e.code[i]);
    }

    tree_tokenizer_init();
    tree_parse(src);
}
