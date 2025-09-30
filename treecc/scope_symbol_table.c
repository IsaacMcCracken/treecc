#include "node.h"



U64 tree_symbol_hash(String s) {
    U64 hash = 1469598103934665603ull;
    for (U64 i = 0; i < s.len; i++) {
        hash ^= (Byte)s.str[i];
        hash *= 1099511628211ull;
    }

    return hash;
}


TreeScopeManager tree_scope_manager_init(Arena *arena, U64 default_cap) {
    return (TreeScopeManager){
        .arena = arena,
        .default_cap = default_cap,
    };
}

TreeScopeTable *tree_alloc_scope_with_cap(TreeScopeManager *m, U64 cap, TreeScopeTable *prev) {
    TreeScopeTable *scope = arena_push(m->arena, TreeScopeTable);
    TreeScopeSymbolCell **cells = arena_push_array(m->arena, TreeScopeSymbolCell*, cap);
    scope->cells = cells;
    scope->capacity = cap;
    scope->prev = prev;

    return scope;
}

TreeScopeTable *tree_alloc_scope(TreeScopeManager *m, TreeScopeTable *prev) {
    if (m->scopepool) {
        // pop off free list
        TreeScopeTable *new_scope = m->scopepool;
        m->scopepool = new_scope->prev;
        // add prev scope
        new_scope->prev = prev;
        // capacity stays the same
        // zero everything else
        new_scope->head = 0;
        new_scope->tail = 0;
        new_scope->symbol_count = 0;
        mem_zero(new_scope->cells, sizeof(TreeScopeSymbolCell*) * new_scope->capacity);
        return new_scope;
    } else {
        return tree_alloc_scope_with_cap(m, m->default_cap, prev);
    }
}

TreeScopeTable *tree_duplicate_scope(TreeScopeManager *m, TreeScopeTable *original) {
    TreeScopeTable *dup_scope = tree_alloc_scope_with_cap(m, original->capacity, 0);
    TreeScopeSymbolCell *cell = original->head;
    while (cell) {
        tree_scope_insert_symbol(m, dup_scope, cell->name, cell->node);
        cell = cell->next;
    }

    if (original->prev) dup_scope->prev = tree_duplicate_scope(m, original->prev);

    return dup_scope;
}

TreeScopeSymbolCell *tree_scope_symbol_cell_alloc(TreeScopeManager *m) {
    if (m->cellpool) {
        TreeScopeSymbolCell *cell =  m->cellpool;
        mem_zero_item(cell, TreeScopeSymbolCell);
        return cell;
    }

    TreeScopeSymbolCell *cell = arena_push(m->arena, TreeScopeSymbolCell);
    return cell;
}

// TreeNode *tree_merge_scopes

void tree_free_all_scopes(TreeScopeManager *m, TreeScopeTable *s) {
    TreeScopeTable *prev = s->prev;
    if (prev) tree_free_all_scopes(m, prev);
    tree_free_single_scope(m, s);
}

void tree_free_single_scope(TreeScopeManager *m, TreeScopeTable *s) {
    // take all the the symbols and push them onto the free list
    TreeScopeSymbolCell *cell = s->head;
    while (cell) {
        TreeScopeSymbolCell *next = cell->next;

        // push cell onto the cell free list
        cell->next = m->cellpool;
        m->cellpool = cell;

        // iterate
        cell = next;
    }


    // push s on to the scope free list
    s->prev = m->scopepool;
    m->scopepool = s;
}


TreeScopeSymbolCell *tree_scope_lookup_symbol_cell(TreeScopeTable *s, String name) {
    U64 slotidx = tree_symbol_hash(name) %s->capacity;
    TreeScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (string_cmp((*cell)->name, name) == 0) {
            return (*cell);
        }

        cell = &(*cell)->hash_next;
    }

    if (s->prev) return tree_scope_lookup_symbol_cell(s->prev, name);

    return 0;
}


TreeNode *tree_scope_lookup_symbol(TreeScopeTable *s, String name) {
    TreeScopeSymbolCell *cell = tree_scope_lookup_symbol_cell(s, name);
    return cell->node;
}

B32 tree_scope_update_symbol(TreeScopeTable *s, String name, TreeNode *node) {
    TreeScopeSymbolCell *cell = tree_scope_lookup_symbol_cell(s, name);
    if (cell) {
        cell->node = node;
        return 1;
    } else {
        return 0;
    }
}

void tree_scope_insert_symbol(TreeScopeManager *m, TreeScopeTable *s, String name, TreeNode *node) {
    U64 slotidx = tree_symbol_hash(name) %s->capacity;
    TreeScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (string_cmp((*cell)->name, name) == 0) {
            (*cell)->node = node;
            return;
        }

        cell = &(*cell)->hash_next;
    }

    TreeScopeSymbolCell *new = arena_push(m->arena, TreeScopeSymbolCell);

    // contents
    new->name = name;
    new->node = node;

    // chain interability
    if (s->tail) {
        s->tail->next = new;
        s->tail = new;
    } else {
        s->head = new;
        s->tail = new;
    }

    *cell = new;

}
