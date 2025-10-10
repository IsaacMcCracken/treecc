#include "node.h"

char *tree_node_get_string_name(File *file, TreeNode *node, char *buf) {
    switch (node->kind) {
        case TreeNodeKind_Return:

    }
}

char *tree_graphviz_visit_node(FILE *file, TreeNodeMap *visited, TreeNode *node) {
    if (tree_map_lookup(visited, node)) {
        return;
    }

    char buf[144] = {0};
    const char *name = NULL;
    switch (node->kind) {
        case TreeNodeKind_Return: {
            name = "return"
        } break;

    }

    tree_map_insert(visited, node);
}

void tree_graphviz_function_output(const char *file, TreeFunction *fn) {
    FILE *f = fopen(file, "w");
    Arena *arena = fn->arena
    TempArena temp = temp_arena_begin(arena);

    TreeNodeMap map = tree_map_init(arena, 101); // count nodes in the graph in future

    tree_graphviz_visit_node(f, &map, fn->start);

    temp_arena_end(temp);
    fclose(f);
}
