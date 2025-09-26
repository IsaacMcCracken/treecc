
#include "stdio.h"

#include "treecc/parser.h"

// remove this
#include "treecc/x64.c"

char *src = "int square(int x) {int c = x - x; return c;}";


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

    TreeFunctionGraph fn = (TreeFunctionGraph){
        .arena = arena,
        .map = tree_map_init(map_arena, 4093),
    };

    Arena *scope_arena = arena_init(1<<24);
    TreeScopeManager scopes = tree_scope_manager_init(scope_arena, 101);

    TreeScopeTable *current_scope = tree_alloc_scope(&scopes, 0);

    TreeParser p = {
        .arena = arena,
        .tokens = tokens,
        .tokencount = tokcount,
        .types = types,
        .src = (Byte*)src,
        .fn = fn,
        .scopes = scopes,
        .current_scope = current_scope,
    };

    // create learning topics


    TreeDecl *decl = tree_parse_decl(&p);
    TreeFnDecl *fndecl = (TreeFnDecl*)decl;
    return p;
}

int main(int argc, char **argv) {

    tree_tokenizer_init();
    TreeParser p = tree_parse(src);

    Arena *arena = arena_init(1<<12);
    X64Emiter e = x64_emiter_init(arena);

    tree_node_print_expr_debug(p.ret);
    putchar('\n');
    cgx64_naive_return(&e, p.ret);


    // WHY testing
    // TreeNode *arg0 = tree_create_proj(p->n)

    for (U32 i = 0; i < e.len; i++) {
        printf("0x%02X, ", e.code[i]);
    }
    putchar('\n');


    int (*fp)(int) = (int(*)(int))e.code;
    os_protect(arena, e.len + sizeof(Arena), OSMemoryFlags_Exec);
    S64 in = 32;
    S64 out = fp(in);
    printf("fp(%ld) = %ld\n", in, out);


    return 0;
}
