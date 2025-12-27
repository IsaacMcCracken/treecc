#include "node.h"
#include "parser.h"

void tree_node_alloc_inputs(TreeFunctionGraph *fn, TreeNode *node, U16 cap);
U16 tree_node_append_input(TreeFunctionGraph *fn, TreeNode *node, TreeNode *input);

U64 tree_symbol_hash(String s) {
    U64 hash = 1469598103934665603ull;
    for (U64 i = 0; i < s.len; i++) {
        hash ^= (Byte)s.str[i];
        hash *= 1099511628211ull;
    }

    return hash;
}


TreeScopeManager tree_scope_manager_init(U64 default_cap) {
    return (TreeScopeManager){
        .default_cap = default_cap,
    };
}

TreeScopeNode *tree_alloc_scope(TreeFunctionGraph *fn) {
    TreeScopeManager *m = &fn->mscope;
    if (m->scopepool) {
        // pop off free list
        TreeScopeNode *new_scope = m->scopepool;
        m->scopepool = new_scope->prev;
        // capacity stays the same
        // zero everything else
        new_scope->prev = 0;;
        new_scope->head = 0;
        new_scope->tail = 0;
        new_scope->symbol_count = 0;
        mem_zero(new_scope->cells, sizeof(TreeScopeSymbolCell*) * new_scope->capacity);
        return new_scope;
    } else {
        TreeScopeSymbolCell **cells = arena_push_array(fn->arena, TreeScopeSymbolCell*, m->default_cap);
        scope->cells = cells;
        scope->capacity = cap;
        scope->prev = prev;

        return scope;
    }
}

TreeNode *tree_push_new_scope(TreeFunctionGraph *fn, TreeNode *ctrl, TreeNode *prev) {
    TreeNode n = (TreeNode){
        .kind = TreeNodeKind_SymbolTable,
    }

    TreeScopeNode *s = tree_alloc_scope(fn);
    n.vptr = (void*)s;

    tree_node_alloc_inputs(fn, &n, (fn->mscope.default_cap * 2)/3);
    
}


TreeNode *tree_duplicate_scope(TreeFunctionGraph *fn, TreeNode *original) {
    TreeNode *dup_node = tree_push_new_scope(fn, original->inputs[0], 0);
    TreeScopeNode *dup_scope = (TreeScopeNode*)dup_node->vptr;
    
    TreeScopeNode *original_scope = (TreeScopeNode*)original->vptr;
    TreeScopeSymbolCell *cell = original_scope->head;
    while (cell) {
        tree_scope_insert_symbol(p, dup, cell->name, original->inputs[cell->slot]);
        cell = cell->next;
    }

    if (original->prev) dup_scope->prev = tree_duplicate_scope(p, original->prev);

    return dup_scope;
}

TreeScopeSymbolCell *tree_scope_symbol_cell_alloc(TreeFunctionGraph *fn) {
    TreeScopeManager *m = &fn->mscope;
    if (m->cellpool) {
        // TODO FOUND BIG BUG
        TreeScopeSymbolCell *cell =  m->cellpool;
        mem_zero_item(cell, TreeScopeSymbolCell);
        return cell;
    }

    TreeScopeSymbolCell *cell = arena_push(p->arena, TreeScopeSymbolCell);
    return cell;
}

// TreeNode *tree_merge_scopes

void tree_free_all_scopes(TreeFunctionGraph *fn, TreeScopeNode *s) {
    TreeScopeManager *m = &fn->mscope;
    TreeScopeNode *prev = s->prev;
    if (prev) tree_free_all_scopes(p, prev);
    tree_free_single_scope(p, s);
}

void tree_free_single_scope(TreeFunctionGraph *fn, TreeScopeNode *s) {
    TreeScopeManager *m = &fn->mscope;
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


TreeScopeSymbolCell *tree_scope_lookup_symbol_cell(TreeNode *scope, String name) {
    TreeScopeNode *s = (TreeScopeNode*)scope->vptr
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


TreeNode *tree_scope_lookup_symbol(TreeNode *scope, String name) {
    TreeScopeSymbolCell *cell = tree_scope_lookup_symbol_cell(scope, name);

    return scope->inputs[cell->slot];
}

B32 tree_scope_update_symbol(TreeNode *scope, String name, TreeNode *node) {
    TreeScopeSymbolCell *cell = tree_scope_lookup_symbol_cell(scope, name);
    if (cell) {
        scope->inputs[cell->slot] = node;
        return 1;
    } else {
        return 0;
    }
}

B32 tree_scope_insert_symbol(TreeFunctionGraph *fn, TreeNode *scope, String name, TreeNode *node) {
    TreeScopeNode *s = (TreeScopeNode*)s->vptr;

    U64 slotidx = tree_symbol_hash(name) %s->capacity;
    TreeScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table do a error;
        if (string_cmp((*cell)->name, name) == 0) {         
            return 0;
        }

        cell = &(*cell)->hash_next;
    }

    TreeScopeSymbolCell *new = arena_push(fn->arena, TreeScopeSymbolCell);

    // contents
    new->name = name;
    new->slot = tree_node_append_input(fn, s, node);

    // chain interability
    if (s->tail) {
        s->tail->next = new;
        s->tail = new;
    } else {
        s->head = new;
        s->tail = new;
    }

    *cell = new;

    return 1;
}

TreeScopeNode *tree_merge_scopes(TreeFunctionGraph *fn, TreeNode *region, TreeNode *this, TreeNode *that) {
    assert(that);

    TreeScopeNode *this_scope = (TreeScopeNode*)this->vptr;
    TreeScopeNode *that_scope = (TreeScopeNode*)that->vptr;

    TreeScopeSymbolCell *cell = this_scope->head;
    while (cell) {
        TreeNode *this_node = cell->node;
        TreeNode *that_node = tree_scope_lookup_symbol(that, cell->name);

        if (this_node != that_node) {
            // create phi
            TreeNode *phi = tree_create_phi2(fn, region, this_node, that_node);
            printf("PHI: %.*s\n", (int)cell->name.len, cell->name.str);
            tree_scope_update_symbol(this, cell->name, phi);
        }

        cell = cell->next;
    }

    if (this->prev) {
        tree_merge_scopes(fn, region, this->prev, that->prev);
    }


    return this;
}
