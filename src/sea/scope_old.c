#include "sea.h"


SeaNode *sea_create_scope(SeaFunctionGraph *fn, SeaNode *prev) {
    SeaScopeNode *s = push_item(fn->arena, SeaScopeNode);
    s->prev = prev;
    U64 cap = fn->mscope.default_cap;
    s->cells = push_array(fn->arena, SeaScopeSymbolCell*, cap)
    s->capacity =
}

SeaScopeManager sea_scope_manager_init(Arena *arena, U64 default_cap) {
    return (SeaScopeManager){
        .arena = arena,
        .default_cap = default_cap,
    };
}

SeaNode *sea_alloc_scope_with_cap(SeaFunctionGraph *fn, U64 cap, SeaNode *prev) {
    SeaScopeManager *m = &fn->mscope;
    SeaNode *scope = arena_push(p->arena, SeaNode);
    SeaScopeSymbolCell **cells = arena_push_array(p->arena, SeaScopeSymbolCell*, cap);
    scope->cells = cells;
    scope->capacity = cap;
    scope->prev = prev;

    return scope;
}

SeaNode *sea_alloc_scope(SeaFunctionGraph *fn, SeaNode *prev) {
    SeaScopeManager *m = &fn->mscope;
    if (m->scopepool) {
        // pop off free list
        SeaNode *new_scope = m->scopepool;
        m->scopepool = new_scope->prev;
        // add prev scope
        new_scope->prev = prev;
        // capacity stays the same
        // zero everything else
        new_scope->head = 0;
        new_scope->tail = 0;
        new_scope->symbol_count = 0;
        mem_zero(new_scope->cells, sizeof(SeaScopeSymbolCell*) * new_scope->capacity);
        return new_scope;
    } else {

        return sea_alloc_scope_with_cap(p, m->default_cap, prev);
    }
}

SeaNode *sea_alloc_scope_region(SeaFunctionGraph *fn, SeaNode *prev, SeaNode *region) {
    SeaScopeManager *m = &fn->mscope;
    SeaNode *scope = sea_alloc_scope(p, prev);
    // scope->region = region;

    return scope;
}

SeaNode *sea_duplicate_scope(SeaFunctionGraph *fn, SeaNode *original) {
    SeaScopeManager *m = &fn->mscope;
    SeaNode *dup_scope = sea_alloc_scope_with_cap(p, original->capacity, 0);
    SeaScopeSymbolCell *cell = original->head;
    while (cell) {
        sea_scope_insert_symbol(p, dup_scope, cell->name, cell->node);
        cell = cell->next;
    }

    if (original->prev) dup_scope->prev = sea_duplicate_scope(p, original->prev);

    return dup_scope;
}

SeaScopeSymbolCell *sea_scope_symbol_cell_alloc(SeaFunctionGraph *fn) {
    SeaScopeManager *m = &fn->mscope;
    if (m->cellpool) {
        SeaScopeSymbolCell *cell =  m->cellpool;
        mem_zero_item(cell, SeaScopeSymbolCell);
        return cell;
    }

    SeaScopeSymbolCell *cell = arena_push(p->arena, SeaScopeSymbolCell);
    return cell;
}

// SeaNode *sea_merge_scopes

void sea_free_all_scopes(SeaFunctionGraph *fn, SeaNode *scope) {
    SeaScopeNode *scopedata = scope->vptr;
    SeaNode *prev = scopedata->prev;
    if (prev) sea_free_all_scopes(fn, prev);
    sea_pop_scope(p, s);
}

void sea_pop_scope(SeaFunctionGraph *fn, SeaNode *s) {
    SeaScopeManager *m = &fn->mscope;
    // take all the the symbols and push them onto the free list
    SeaScopeSymbolCell *cell = s->head;
    while (cell) {
        SeaScopeSymbolCell *next = cell->next;

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


SeaScopeSymbolCell *sea_scope_lookup_symbol_cell(SeaNode *s, String8 name) {
    U64 slotidx = sea_u64_hash_from_str8(name) %s->capacity;
    SeaScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (string_cmp((*cell)->name, name) == 0) {
            return (*cell);
        }

        cell = &(*cell)->hash_next;
    }

    if (s->prev) return sea_scope_lookup_symbol_cell(s->prev, name);

    return 0;
}


SeaNode *sea_scope_lookup_symbol(SeaNode *s, String8 name) {
    SeaScopeSymbolCell *cell = sea_scope_lookup_symbol_cell(s, name);

    return cell->node;
}

B32 sea_scope_update_symbol(SeaNode *s, String8 name, SeaNode *node) {
    SeaScopeSymbolCell *cell = sea_scope_lookup_symbol_cell(s, name);
    if (cell) {
        cell->node = node;
        return 1;
    } else {
        return 0;
    }
}

void sea_scope_insert_symbol(SeaFunctionGraph *fn, SeaNode *s, String8 name, SeaNode *node) {
    SeaScopeManager *m = &fn->mscope;

    U64 slotidx = sea_u64_hash_from_str8(name) %s->capacity;
    SeaScopeSymbolCell **cell = &s->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (string_cmp((*cell)->name, name) == 0) {
            (*cell)->node = node;
            return;
        }

        cell = &(*cell)->hash_next;
    }

    SeaScopeSymbolCell *new = arena_push(p->arena, SeaScopeSymbolCell);

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

SeaNode *sea_merge_scopes(SeaFunctionGraph *fn, SeaNode *region, SeaNode *scope_a, SeaNode *scope_b) {
    assert(that);

    SeaScopeNode *this = scope_a->vptr;
    SeaScopeNode *that = scope_b->vptr;

    SeaScopeSymbolCell *cell = this->head;
    while (cell) {
        SeaNode *this_node = cell->node;
        SeaNode *that_node = sea_scope_lookup_symbol(that, cell->name);

        if (this_node != that_node) {
            // create phi
            SeaNode *phi = sea_create_phi2(fn, region, this_node, that_node);
            printf("PHI: %.*s\n", (int)cell->name.len, cell->name.str);
            sea_scope_update_symbol(this, cell->name, phi);
        }

        cell = cell->next;
    }

    if (this->prev) {
        sea_merge_scopes(fn, region, this->prev, that->prev);
    }


    return this;
}

// SeaNode *sea_create_symbol_table(SeaFunctionGraph *fn) {
//     SeaNode n {
//         .kind = SeaNodeKind_SymbolTable,
//     };
// }
