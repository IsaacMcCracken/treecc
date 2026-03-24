#include <sea/sea.h>
#include <sea/sea_internal.h>


SeaModule sea_create_module(void) {
    // TODO create symbol table
    Arena *arena = arena_alloc(.reserve_size = MB(64));
    U64 cap = 401;
    SeaSymbolEntry **cells = push_array(arena, SeaSymbolEntry *, cap);

    SeaSymbols symbols = (SeaSymbols) {
        .arena = arena,
        .cap = cap,
        .cells = cells
    };


    Arena *emit_arena = arena_alloc(.reserve_size = GB(2));

    SeaModule m = (SeaModule){
        .lock = rw_mutex_alloc(),
        .symbols = symbols,
        .emit = emitter_init(emit_arena),
    };

    sea_lattice_init(&m);



    return m;
}

// @public
SeaSymbolEntry *sea_lookup_symbol(SeaModule *m, String8 name) {
    SeaSymbolEntry *result = 0;

    rw_mutex_take_r(m->lock);
    U64 hash = u64_hash_from_str8(name);
    U64 idx = hash % m->symbols.cap;
    SeaSymbolEntry **cell = &m->symbols.cells[idx];

    while (*cell) {
        if (str8_match((*cell)->name, name, 0)) {
            result = (*cell);
            break;
        }
        *cell = (*cell)->next_hash;
    }

    rw_mutex_drop_r(m->lock);

    return result;
}

B32 sea_set_symbol_pos(SeaModule *m, String8 name, U64 pos) {
    B32 result = 0;
    rw_mutex_take_r(m->lock);
    U64 hash = u64_hash_from_str8(name);
    U64 idx = hash % m->symbols.cap;
    SeaSymbolEntry **cell = &m->symbols.cells[idx];

    while (*cell) {
        if (str8_match((*cell)->name, name, 0)) {
            (*cell)->pos_in_section = pos;
            result = 1;
            break;
        }
        *cell = (*cell)->next_hash;
    }

    rw_mutex_drop_r(m->lock);
    return result;
}

// @private
B32 sea_add_symbol(SeaModule *m, SeaSymbolEntry *e) {
    U64 hash = u64_hash_from_str8(e->name);
    U64 idx = hash % m->symbols.cap;

    SeaSymbolEntry **cell = &m->symbols.cells[idx];

    while (*cell) {
        if (str8_match((*cell)->name, e->name, 0)) {
            return 0;
        }
        *cell = (*cell)->next_hash;
    }

    e->next_hash = *cell;
    *cell = e;

    SLLQueuePush(m->symbols.first, m->symbols.last, e);

    m->symbols.count += 1;
}

void sea_add_function_symbol_(SeaModule *m, SeaFunctionProto proto) {
    SeaSymbolEntry *e = push_item(m->symbols.arena, SeaSymbolEntry);

    SeaType *t = sea_type_make_func(m, proto);
    e->type = t;
    e->name = proto.name;
    sea_add_symbol(m, e);
}

void sea_add_function_symbol(SeaModule *m, SeaFunctionProto proto) {
    rw_mutex_take(m->lock, 1);

    sea_add_function_symbol_(m, proto);

    rw_mutex_drop(m->lock, 1);
}

void sea_add_struct_symbol(SeaModule *m, SeaTypeStruct s) {
    rw_mutex_take_w(m->lock);

    SeaSymbolEntry *e = push_item(m->symbols.arena, SeaSymbolEntry);
    e->name = s.name;
    e->type = sea_type_make_struct(m, s);

    sea_add_symbol(m, e);

    rw_mutex_drop_w(m->lock);
}




SeaFunctionGraph *sea_add_function(
    SeaModule *mod,
    SeaScopeManager *m,
    SeaFunctionProto proto
) {
    Arena *arena = arena_alloc(.reserve_size = MB(1)); // TODO heuristic for how much

    // This code is ugly as fuck;
    SeaAllocator *alloc = push_item(arena, SeaAllocator);
    // Yippee
    SeaFunctionGraphNode *fn_node = push_item(arena, SeaFunctionGraphNode);
    SeaFunctionGraph *fn = &fn_node->fn;

    // Set intenals
    {
        fn->arena = arena;
        fn->proto = proto;
        fn->m = mod;
        fn->alloc = alloc;
        fn->map = sea_map_init(arena, 101);

        sea_push_scope(m);
        fn->start = sea_create_start(fn, m, proto);
        fn->stop = sea_create_stop(fn, 4);
    }


    // add the function to the module list
    {
        rw_mutex_take(mod->lock, 1);

        SLLQueuePush(mod->functions.first, mod->functions.last, fn_node);
        mod->functions.count += 1;

        sea_add_function_symbol_(mod, proto);

        rw_mutex_drop(mod->lock, 1);
    }

    return fn;
}
