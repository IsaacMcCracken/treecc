#ifndef TREE_SCOPE_SYMBOL_TABLE_H
#define TREE_SCOPE_SYMBOL_TABLE_H



U64 tree_symbol_hash(String s);
TreeScopeManager tree_scope_manager_init(TreeFunctionGraph *fn, U64 default_cap);
TreeNode *tree_create_scope(TreeFunctionGraph *fn, TreeNode *prev);
TreeScopeNode *tree_duplicate_scope(TreeFunctionGraph *fn, TreeScopeNode *original);
TreeScopeNode *tree_merge_scopes(TreeFunctionGraph *fn, TreeNode *region, TreeScopeNode *this, TreeScopeNode *that);
TreeScopeSymbolCell *tree_scope_symbol_cell_alloc(TreeFunctionGraph *fn);
void tree_free_single_scope(TreeFunctionGraph *fn, TreeScopeNode *s);
void tree_free_all_scopes(TreeFunctionGraph *fn, TreeScopeNode *s);
void tree_scope_insert_symbol(TreeFunctionGraph *fn, TreeScopeNode *s, String name, TreeNode *node);
TreeNode *tree_scope_lookup_symbol(TreeScopeNode *s, String name);
B32 tree_scope_update_symbol(TreeScopeNode *s, String name, TreeNode *node);

#endif
