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
    return tree_alloc_scope_with_cap(m, m->default_cap, prev);
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

void tree_free_scope(TreeScopeManager *m, TreeScopeTable *s) {
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


TreeNode *tree_scope_lookup_symbol(TreeScopeTable *s, String name) {
    U64 slotidx = tree_symbol_hash(name) %s->capacity;
    TreeScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (string_cmp((*cell)->name, name) == 0) {
            return (*cell)->node;
        }

        cell = &(*cell)->hash_next;
    }

    if (s->prev) return tree_scope_lookup_symbol(s->prev, name);

    return 0;
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
