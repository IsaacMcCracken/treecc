
#include "sea.h"
#include "sea_internal.h"

// For the fourth iteration the scope manager is in the users parser data
// rather than the function since this is not needed after parsing
// but then also at the same time scope needs many function graph usage so -\(^O^)/-
/**
 * TODO
 * [x] - symbol_cell_alloc
 * [x] - symbol_cell_free
 * [x] - data_alloc
 * [x] - data_free
 * [x] - scope_free // frees all symbolcells and datas
 * [x] - push_scope
 * [x] - pop_scope
 * [x] - insert
 * [x] - update
 * [x] - lookup
 * [x] - create_scope
 * [ ] - merge_scopes
 * [ ] - duplicate_scope
 * [ ] - end_loop
 */

 // TODO-today
 // move all allocations to the parser
 // Get rid of mscope and scope from function
 //

String8 *sea_scope_get_all_names(Arena *arena, SeaScopeManager *m, U64 *count) {
    SeaNode *scope = m->curr;
    SeaScopeList *list = scope->vptr;

    String8 *names = push_array(arena, String8, scope->inputlen);
    *count = scope->inputlen;

    U64 i = 0;
    for EachNode(data, SeaScopeData, list->head) {
        for EachNode(cell, SeaScopeSymbolCell, data->head) {
            names[i] = cell->name;
            i += 1;
        }
    }

    Assert(i == *count);
}

SeaScopeSymbolCell *symbol_cell_alloc(SeaScopeManager *m) {
    if (m->cellpool) {
        SeaScopeSymbolCell *cell =  m->cellpool;
        m->cellpool = cell->next;
        MemoryZeroStruct(cell);
        return cell;
    }

    SeaScopeSymbolCell *cell = push_item(m->arena, SeaScopeSymbolCell);
    return cell;
}

void symbol_cell_free(SeaScopeManager *m, SeaScopeSymbolCell *cell) {
    cell->next = m->cellpool;
    m->cellpool = cell;
}

void push_symbol_cell(SeaScopeData *data, SeaScopeSymbolCell *cell) {
    if (data->tail) {
        data->tail->next = cell;
        data->tail = cell;
    } else {
        data->head = cell;
        data->tail = cell;
    }
}


SeaScopeData *scope_data_alloc( SeaScopeManager *m) {
    if (m->scopepool) {
        // Pop off freelist
        SeaScopeData *data = m->scopepool;
        m->scopepool = data->next;

        SeaScopeSymbolCell **cells = data->cells;
        U64 cap = data->cap;

        MemoryZeroStruct(data);
        MemoryZeroTyped(cells, cap);

        data->cap = cap;
        data->cells = cells;

        return data;
    }

    SeaScopeData *data = push_item(m->arena, SeaScopeData);

    SeaScopeSymbolCell **cells = push_array(
        m->arena,
        SeaScopeSymbolCell*,
        m->default_cap
    );

    data->cells = cells;
    data->cap = m->default_cap;
    return data;
}

void scope_data_free(SeaScopeManager *m, SeaScopeData *data) {
    SeaScopeSymbolCell *cell = data->head;
    while (cell) {
        SeaScopeSymbolCell *next = cell->next;
        symbol_cell_free(m, cell);
        cell = next;
    }
    data->next = m->scopepool;
    m->scopepool = data;
}

void sea_scope_free(SeaFunctionGraph *fn, SeaScopeManager *m, SeaNode *scope) {
    fprintf(stderr, "sea_scope_free called on %p\n", scope);
    // remove inputs first so user lists are clean
    for EachIndex(i, scope->inputlen) {
        sea_node_set_input(fn, scope, 0, i);
    }
    // then free scope metadata
    SeaScopeList *l = scope->vptr;
    SeaScopeData *n = l->head;
    while (n) {
        SeaScopeData *next = n->next;
        scope_data_free(m, n);
        n = next;
    }
}

void sea_push_scope(SeaScopeManager *m) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;

    SeaScopeData *data = scope_data_alloc(m);
    if (l->tail) {
        l->tail->next = data;
        data->prev = l->tail;
        l->tail = data;
    } else {
        l->head = data;
        l->tail = data;
    }

    data->inputlen = scope->inputlen;
}

void sea_pop_scope(SeaFunctionGraph *fn, SeaScopeManager *m) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;
    SeaScopeData *popped = l->tail;
    U16 oldlen = popped->inputlen;
    for EachIndexFrom(i, oldlen, scope->inputlen) {
        sea_node_set_input(fn, scope, 0, i);
    }
    scope->inputlen = popped->inputlen;
    l->tail = popped->prev;
    if (l->tail) {
        l->tail->next = 0; // sever the forward pointer
    } else {
        l->head = 0; // list is now empty
    }
    scope_data_free(m, popped);
}


void sea_begin_scope(SeaScopeManager *m) {
    SeaNode *scope = sea_create_scope(m, 512);
    m->curr = scope;
}

void sea_end_scope(SeaFunctionGraph *fn, SeaScopeManager *m) {
    SeaNode *s = m->curr;
    for EachIndex(i, s->inputlen) {
        sea_node_set_input(fn, s, 0, i);
    }
    m->curr = 0;
    m->cellpool = 0;
    m->scopepool = 0;
}


void sea_scope_insert_symbol(
    SeaFunctionGraph *fn,
    SeaScopeManager *m,
    String8 name,
    SeaNode *node
) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;
    SeaScopeData *data = l->tail;

    U64 slotidx = u64_hash_from_str8(name) % data->cap;
    SeaScopeSymbolCell **cell = &data->cells[slotidx];

    while (*cell) {
        // if its already in the table update it.
        if (str8_match((*cell)->name, name, 0)) {
            // replace the slot
            sea_node_set_input(fn, scope, node, (*cell)->slot);
            return;
        }
        cell = &(*cell)->hash_next;
    }


    SeaScopeSymbolCell *new = symbol_cell_alloc(m);
    // contents
    new->name = name;
    new->slot = sea_node_append_input(fn, scope, node);

    push_symbol_cell(data, new);

    *cell = new;
}


SeaNode *sea_scope_lookup_symbol(SeaScopeManager *m, String8 name) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;
    SeaScopeData *data = l->tail;

    while (data) {
        U64 slotidx = u64_hash_from_str8(name) %data->cap;
        SeaScopeSymbolCell **cell = &data->cells[slotidx];
        while (*cell) {
            // if its already in the table update it.
            if (str8_match((*cell)->name, name, 0)) {
                return scope->inputs[(*cell)->slot];
            }

            cell = &(*cell)->hash_next;
        }
        data = data->prev;
    }

    return 0;
}


void sea_scope_update_symbol(
    SeaFunctionGraph *fn,
    SeaScopeManager *m,
    String8 name,
    SeaNode *node
) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;
    SeaScopeData *data = l->tail;



    while (data) {
        U64 slotidx = u64_hash_from_str8(name) %data->cap;
        SeaScopeSymbolCell **cell = &data->cells[slotidx];
        while (*cell) {
            if (str8_match((*cell)->name, name, 0)) {
                sea_node_set_input(fn, scope, node, (*cell)->slot);
                return;
            }

            cell = &(*cell)->hash_next;
        }
        data = data->prev;
    }
}




SeaNode *sea_duplicate_scope(SeaFunctionGraph *fn, SeaScopeManager *m, B32 isloop) {
    // Its not as spaghetti anymore
    SeaNode *original = m->curr;
    SeaNode *ctrl = original->inputs[0];
    SeaScopeList *l = original->vptr;

    SeaNode *dup = sea_create_scope(m, original->inputcap);
    m->curr = dup;

    for EachNode(data, SeaScopeData, l->head) {
        sea_push_scope(m);
        for EachNode(cell, SeaScopeSymbolCell, data->head) {
            U16 slot = dup->inputlen;
            Assert(slot == cell->slot);
            SeaNode *node = original->inputs[cell->slot];
            if (isloop && cell->slot != 0) {
                SeaNode *phi = sea_create_phi2(fn, ctrl, node, 0);
                sea_scope_insert_symbol(fn, m, cell->name, phi);
                sea_node_set_input(fn, original, phi, slot);
            } else {
                sea_scope_insert_symbol(fn, m, cell->name, node);
            }
        }
    }

    m->curr = original;
    return dup;
}


/**
 * Merges that_scope into this_scope returns a new region
 */
SeaNode *sea_merge_scopes(
    SeaFunctionGraph *fn,
    SeaScopeManager *m,
    SeaNode *that_scope
) {
    SeaNode *this_scope = m->curr;
    Assert(this_scope->inputlen == that_scope->inputlen);

    SeaNode *this_ctrl = this_scope->inputs[0];
    SeaNode *that_ctrl = that_scope->inputs[0];

    SeaNode *region_inputs[] = {0, this_ctrl, that_ctrl};
    SeaNode *region = sea_create_region(fn, region_inputs, 3);
    sea_node_keep(fn, region);


    for EachIndexFrom(i, 1, this_scope->inputlen) {
        SeaNode *this_node = this_scope->inputs[i];
        SeaNode *that_node = that_scope->inputs[i];

        if (this_node != that_node) {
            SeaNode *phi = sea_create_phi2(fn, region, this_node, that_node);
            sea_node_set_input(fn, this_scope, phi, i);
        }
    }


    fprintf(stderr, "freeing scope %p, inputlen=%u\n", that_scope, that_scope->inputlen);
    for EachIndex(i, that_scope->inputlen) {
        fprintf(stderr, "  input[%u] = %p\n", i, that_scope->inputs[i]);
    }
    sea_scope_free(fn, m, that_scope);

    sea_node_unkeep(fn, region);
    SeaNode *result = sea_peephole(fn, region);

    return result;
}

void sea_scope_end_loop(SeaFunctionGraph *fn, SeaScopeManager *m, SeaNode *head, SeaNode *back, SeaNode *exit) {
    SeaNode *loop = head->inputs[0];
    Assert(loop->kind == SeaNodeKind_Loop && loop->inputs[2] == 0); // TODO maybe last one
    SeaNode *back_ctrl = back->inputs[0];
    sea_node_set_input(fn, loop, back_ctrl, 2); // set back edge

    for EachIndexFrom(i, 1, head->inputlen) {
        SeaNode *phi = head->inputs[i];
        Assert(phi->kind == SeaNodeKind_Phi && phi->inputs[0] == loop);
        SeaNode *other = back->inputs[i];
        if (phi == other) {
            sea_node_subsume(fn, phi, phi->inputs[1]);
        } else {
            sea_node_set_input(fn, phi, other, 2);
        }

        SeaNode *in = sea_peephole(fn, phi);
        if (in != phi) {
            sea_node_subsume(fn, phi, in);
        }
    }

    sea_scope_free(fn, m, back);
    // sea_scope_free(fn, m, head)

    m->curr = exit;
}
