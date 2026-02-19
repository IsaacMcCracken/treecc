
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

void sea_scope_free(SeaScopeManager *m, SeaNode *scope) {
    SeaScopeList *l = scope->vptr;
    SeaScopeData *n = l->head;
    while (n) {
        SeaScopeData *next = n->next;
        scope_data_free(m, n);
        n = next;
    }
    // if garbage collected free the scope node
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

void sea_pop_scope(SeaScopeManager *m) {
    SeaNode *scope = m->curr;
    SeaScopeList *l = scope->vptr;

    SeaScopeData *popped = l->tail;
    scope->inputlen = popped->inputlen;
    l->tail = popped->prev;

    scope_data_free(m, popped);
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
    // Fuck this spaghetti ass code
    SeaNode *original = m->curr;
    SeaScopeList *l = original->vptr;

    SeaNode *dup = sea_create_scope(m, original->inputcap);
    m->curr = dup;

    for EachNode(data, SeaScopeData, l->head) {
        sea_push_scope(m);
        for EachNode(cell, SeaScopeSymbolCell, data->head) {
            U16 slot = dup->inputlen;
            Assert(slot == cell->slot);
            SeaNode *node = original->inputs[cell->slot];
            sea_scope_insert_symbol(fn, m, cell->name, node);
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


    for EachIndexFrom(i, 1, this_scope->inputlen) {
        SeaNode *this_node = this_scope->inputs[i];
        SeaNode *that_node = that_scope->inputs[i];

        if (this_node != that_node) {
            SeaNode *phi = sea_create_phi2(fn, region, this_node, that_node);
            sea_node_set_input(fn, this_scope, phi, i);
        }
    }

    sea_node_set_input(fn, this_scope, region, 0);
    sea_scope_free(m, that_scope);
    sea_peephole(fn, region);

    return region;
}

// void sea_scope_end_loop(SeaFunctionGraph *fn, SeaNode *head, SeaNode *back, SeaNode *exit) {
//     SeaNode *ctrl = sea_scope_lookup_symbol(fn, head, CTRL_STR);
//     Assert(ctrl->kind == SeaNodeKind_Loop && ctrl->inputs[2] == 0);
//     SeaNode *back_ctrl = sea_scope_lookup_symbol(fn, back, CTRL_STR);

//     SeaNode *this_scope = head;
//     SeaNode *that_scope = back;
//     while (this_scope && that_scope) {
//         SeaScopeData *this = this_scope->vptr;
//         SeaScopeData *that = that_scope->vptr;

//         SeaScopeSymbolCell *cell = this->head;
//         while (cell) {
//             if (!str8_match(cell->name, CTRL_STR, 0)) {
//                 SeaNode *this_phi = this_scope->inputs[cell->slot];
//                 Assert(this_phi->kind == SeaNodeKind_Phi && this_phi->inputs[2] == 0 && this_phi->inputs[0] == ctrl);

//                 SeaNode *that_node = sea_scope_lookup_symbol(fn, that_scope, cell->name);

//                 sea_node_set_input(fn, this_phi, that_node, 2);

//                 SeaNode *in = sea_peephole(fn, this_phi);

//                 if (in != this_phi) {
//                     sea_subsume(fn, this_phi, in);
//                 }
//             }

//             cell = cell->next;
//         }

//         this_scope = this->prev;
//         that_scope = that->prev;
//     }

//     sea_free_all_scopes(fn, back);
// }
