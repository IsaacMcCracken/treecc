
#include "sea.h"
#include "sea_internal.h"


SeaNode *sea_alloc_scope_with_cap(SeaFunctionGraph *fn, U64 cap) {
    SeaScopeManager *m = &fn->mscope;
    SeaScopeNode *scopedata = push_item(fn->arena, SeaScopeNode);
    SeaScopeSymbolCell **cells = push_array(fn->arena, SeaScopeSymbolCell*, cap);
    scopedata->cells = cells;
    scopedata->cap = cap;

    // Do not register with the garbage collector prolly
    SeaNode *n = push_item(fn->arena, SeaNode);
    n->kind = SeaNodeKind_Scope;
    n->vptr = scopedata;
    sea_node_alloc_inputs(fn, n, 16);

    return n;
}

void sea_scope_insert_symbol(SeaFunctionGraph *fn, SeaNode *scope, String8 name, SeaNode *node) {
    SeaScopeManager *m = &fn->mscope;
    SeaScopeNode *scopedata = scope->vptr;


    U64 slotidx = u64_hash_from_str8(name) % scopedata->cap;
    SeaScopeSymbolCell **cell = &scopedata->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (str8_match((*cell)->name, name, 0)) {
            // replace the slot
            scope->inputs[(*cell)->slot] = node;
            return;
        }

        cell = &(*cell)->hash_next;
    }

    SeaScopeSymbolCell *new = push_item(fn->arena, SeaScopeSymbolCell);

    // contents
    new->name = name;
    new->slot = sea_node_append_input(fn, scope, node);

    // chain interability
    if (scopedata->tail) {
        scopedata->tail->next = new;
        scopedata->tail = new;
    } else {
        scopedata->head = new;
        scopedata->tail = new;
    }

    *cell = new;
}

void sea_insert_local_symbol(SeaFunctionGraph *fn, String8 name, SeaNode *node) {
    return sea_scope_insert_symbol(fn, fn->scope, name, node);
}


SeaScopeSymbolCell *sea_scope_lookup_symbol_cell(SeaNode *scope, String8 name) {
    SeaScopeNode *scopedata = scope->vptr;
    U64 slotidx = u64_hash_from_str8(name) %scopedata->cap;
    SeaScopeSymbolCell **cell = &scopedata->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (str8_match((*cell)->name, name, 0)) {
            return (*cell);
        }

        cell = &(*cell)->hash_next;
    }

    if (scopedata->prev) return sea_scope_lookup_symbol_cell(scopedata->prev, name);

    return 0;
}


SeaNode *sea_scope_lookup_symbol(SeaFunctionGraph *fn, String8 name) {
    SeaScopeSymbolCell *cell = sea_scope_lookup_symbol_cell(fn->scope, name);
    return fn->scope->inputs[cell->slot];
}


SeaScopeSymbolCell *sea_scope_symbol_cell_alloc(SeaFunctionGraph *fn) {
    SeaScopeManager *m = &fn->mscope;
    if (m->cellpool) {
        SeaScopeSymbolCell *cell =  m->cellpool;
        MemoryZeroStruct(cell);
        return cell;
    }

    SeaScopeSymbolCell *cell = push_item(fn->arena, SeaScopeSymbolCell);
    return cell;
}

SeaNode *sea_duplicate_scope(SeaFunctionGraph *fn, SeaNode *original) {
    SeaScopeManager *m = &fn->mscope;
    SeaScopeNode *scopedata = original->vptr;
    SeaNode *dup_scope = sea_alloc_scope_with_cap(fn, scopedata->cap);
    SeaScopeSymbolCell *cell = scopedata->head;
    while (cell) {
        sea_scope_insert_symbol(fn, dup_scope, cell->name, original->inputs[cell->slot]);
        cell = cell->next;
    }

    SeaScopeNode *dupscopedata = dup_scope->vptr;
    if (scopedata->prev) dupscopedata->prev = sea_duplicate_scope(fn, scopedata->prev);

    return dup_scope;
}

void sea_push_new_scope(SeaFunctionGraph *fn) {
    SeaScopeManager *m = &fn->mscope;
    SeaNode *scope;
    if (m->scopepool) {
        // pop off free list
        SeaNode *n = m->scopepool;
        SeaScopeNode *scopedata = n->vptr;
        m->scopepool = scopedata->prev;
        // add prev scope
        // capacity stays the same
        // zero everything else
        scopedata->head = 0;
        scopedata->tail = 0;
        scopedata->symbol_count = 0;
        MemoryZero(scopedata->cells, sizeof(SeaScopeSymbolCell*) * scopedata->cap);
        scope = n;
    } else {

        scope = sea_alloc_scope_with_cap(fn, m->default_cap);
    }

    SeaScopeNode *scopedata = scope->vptr;
    scopedata->prev = fn->scope;
    fn->scope = scope;
}

void sea_free_all_scopes(SeaFunctionGraph *fn, SeaNode *scope) {
    SeaScopeNode *scopedata = scope->vptr;
    SeaNode *prev = scopedata->prev;
    if (prev) sea_free_all_scopes(fn, prev);
    sea_pop_scope(fn, scope);
}

void sea_pop_scope(SeaFunctionGraph *fn, SeaNode *scope) {
    SeaScopeManager *m = &fn->mscope;
    SeaScopeNode *scopedata = scope->vptr;
    // take all the the symbols and push them onto the free list
    SeaScopeSymbolCell *cell = scopedata->head;
    while (cell) {
        SeaScopeSymbolCell *next = cell->next;

        // push cell onto the cell free list
        cell->next = m->cellpool;
        m->cellpool = cell;

        // iterate
        cell = next;
    }

    fn->scope = scopedata->prev;
    // push s on to the scope free list
    scopedata->prev = m->scopepool;
    m->scopepool = scope;

}
