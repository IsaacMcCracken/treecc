#include <node.h>

char *soup_graphviz_visit_node(FILE *file, SoupNodeMap *visited, SoupNode *node) {
    if (soup_map_lookup(visited, node)) {
        return;
    }

    char buf[144] = {0};
    const char *name = NULL;
    switch (node->kind) {
        case SoupNodeKind_Return: {
            name = "return"
        } break;

    }

    soup_map_insert(visited, node);
}

void soup_graphviz_function_output(const char *file, SoupFunction *fn) {
    FILE *f = fopen(file, "w");
    Arena *arena = fn->arena
    TempArena temp = temp_arena_begin(arena);

    SoupNodeMap map = soup_map_init(arena, 101); // count nodes in the graph in future

    soup_graphviz_visit_node(f, &map, fn->start);

    temp_arena_end(temp);
    fclose(f);
}
