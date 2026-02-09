
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

// TODO we need to update the slot instead
SeaNode *sea_scope_lookup_symbol(SeaFunctionGraph *fn, SeaNode *scope, String8 name) {
    SeaScopeNode *scopedata = scope->vptr;
    U64 slotidx = u64_hash_from_str8(name) %scopedata->cap;
    SeaScopeSymbolCell **cell = &scopedata->cells[slotidx];
    while (*cell) {
        // if its already in the table update it.
        if (str8_match((*cell)->name, name, 0)) {
            return scope->inputs[(*cell)->slot];
        }

        cell = &(*cell)->hash_next;
    }

    if (scopedata->prev) return sea_scope_lookup_symbol(fn, scopedata->prev, name);

    return 0;
}


void sea_scope_update_symbol(SeaFunctionGraph *fn, SeaNode *scope, String8 name, SeaNode *node) {

    while (scope) {
        SeaScopeNode *scopedata = scope->vptr;
        U64 slotidx = u64_hash_from_str8(name) %scopedata->cap;
        SeaScopeSymbolCell **cell = &scopedata->cells[slotidx];
        while (*cell) {
            if (str8_match((*cell)->name, name, 0)) {
                sea_node_set_input(fn, scope, node, (*cell)->slot);
                return;
            }

            cell = &(*cell)->hash_next;
        }
        scope = scopedata->prev;
    }
}

void sea_update_local_symbol(SeaFunctionGraph *fn, String8 name, SeaNode *node) {
    sea_scope_update_symbol(fn, fn->scope, name, node);
}



SeaNode *sea_lookup_local_symbol(SeaFunctionGraph *fn, String8 name) {
    return sea_scope_lookup_symbol(fn, fn->scope, name);
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
        scopedata->depth = 0;
        scopedata->symbol_count = 0;
        MemoryZero(scopedata->cells, sizeof(SeaScopeSymbolCell*) * scopedata->cap);
        scope = n;
    } else {

        scope = sea_alloc_scope_with_cap(fn, m->default_cap);
    }

    SeaScopeNode *scopedata = scope->vptr;
    scopedata->prev = fn->scope;

    if (scopedata->prev) {
        SeaScopeNode *prevscopedata = scopedata->prev->vptr;
        scopedata->depth = prevscopedata->depth + 1;
        printf("Scope(%p) depth of %llu\n", scope);
    }

    if (fn->scope) {
        SeaNode *prev_ctrl = sea_scope_lookup_symbol(fn, fn->scope, CTRL_STR);
        sea_scope_insert_symbol(fn, scope, CTRL_STR, prev_ctrl);
    }

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

/**
 * Merges that_scope into this_scope returns a new region
 */
SeaNode *sea_merge_scopes(
    SeaFunctionGraph *fn,
    SeaNode *this_scope,
    SeaNode *that_scope,
    SeaNode **out_scope
) {


    SeaNode *this_ctrl = sea_scope_lookup_symbol(fn, this_scope, CTRL_STR);
    SeaNode *that_ctrl = sea_scope_lookup_symbol(fn, that_scope, CTRL_STR);

    if (this_ctrl->type == &sea_type_CtrlDead) {
        SeaNode *ifnode = that_ctrl->inputs[0];
        Assert(ifnode->kind == SeaNodeKind_If);
        SeaNode *valid_ctrl = ifnode->inputs[0];
        Assert(valid_ctrl->type == &sea_type_CtrlLive);
        sea_free_all_scopes(fn, this_scope);
        *out_scope = that_scope;
        return valid_ctrl;
    }

    if (that_ctrl->type == &sea_type_CtrlDead) {
        SeaNode *ifnode = this_ctrl->inputs[0];
        Assert(ifnode->kind == SeaNodeKind_If);
        SeaNode *valid_ctrl = ifnode->inputs[0];
        Assert(valid_ctrl->type == &sea_type_CtrlLive);
        sea_free_all_scopes(fn, that_scope);
        *out_scope = this_scope;
        return valid_ctrl;
    }



    SeaNode *ctrl_ins[2] = {this_ctrl, that_ctrl};
    SeaNode *region = sea_create_region(fn, ctrl_ins, 2, 8);

    while (this_scope && that_scope) {

        SeaScopeNode *this = this_scope->vptr;
        SeaScopeNode *that = that_scope->vptr;

        SeaScopeSymbolCell *cell = this->head;
        while (cell) {
            if (!str8_match(cell->name, CTRL_STR, 0)) {
                SeaNode *this_node = this_scope->inputs[cell->slot];
                SeaNode *that_node = sea_scope_lookup_symbol(fn, that_scope, cell->name);

                if (this_node != that_node) {
                    // create phi
                    SeaNode *phi = sea_create_phi2(fn, region, this_node, that_node);
                    // printf("Mergin: %.*s\n", str8_varg(cell->name));
                    sea_scope_update_symbol(fn, this_scope, cell->name, phi);
                }
            }


            cell = cell->next;
        }

        this_scope = this->prev;
        that_scope = that->prev;
    }


    return region;
}


void sea_pop_this_scope(SeaFunctionGraph *fn) {
    sea_pop_scope(fn, fn->scope);
}
