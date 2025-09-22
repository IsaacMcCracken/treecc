#ifndef TREE_SCOPE_SYMBOL_TABLE_H
#define TREE_SCOPE_SYMBOL_TABLE_H


/*
 * The TreeScopeManager should have its own arena and
 *
 */

typedef struct TreeScopeSymbolCell TreeScopeSymbolCell;
struct TreeScopeSymbolCell {
    TreeScopeSymbolCell *hash_next; // next in hash buck
    TreeScopeSymbolCell *next; // next symbol inserted (for iteration)
    String name;
    TreeNode *node;
};

typedef struct TreeScopeTable TreeScopeTable;
struct TreeScopeTable {
  TreeScopeTable *prev; // previous scope / parent
  TreeScopeSymbolCell **cells; // buckets for table lookup
  U64 capacity; // capacity
  TreeScopeSymbolCell *head; // first symbol (for iteration)
  TreeScopeSymbolCell *tail; // last symbol (for appending)
  U64 symbol_count;
};

typedef struct TreeScopeManager TreeScopeManager;
struct TreeScopeManager {
    TreeScopeSymbolCell *cellpool; // free list for cells
    TreeScopeTable *scopepool; // free list for scopes
    Arena *arena; // arena for allocating scope and cells
    U64 default_cap; // capacity for new scopes
};






U64 tree_symbol_hash(String s);
TreeScopeManager tree_scope_manager_init(Arena *arena, U64 default_cap);
TreeScopeTable *tree_alloc_scope(TreeScopeManager *m, TreeScopeTable *prev);
TreeScopeSymbolCell *tree_scope_symbol_cell_alloc(TreeScopeManager *m);
void tree_free_scope(TreeScopeManager *m, TreeScopeTable *s);
void tree_scope_insert_symbol(TreeScopeManager *m, TreeScopeTable *s, String name, TreeNode *node);
TreeNode *tree_scope_lookup_symbol(TreeScopeTable *s, String name);

#endif
