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

    return (SeaModule){
        .lock = rw_mutex_alloc(),
        .symbols = symbols,
    };
}

void sea_add_symbol(SeaModule *m, SeaSymbolEntry *e) {
    U64 hash = u64_hash_from_str8(e->name);
    U64 idx = hash % m->symbols.cap;

    SeaSymbolEntry **cell = &m->symbols.cells[idx];

    e->next_hash = *cell;
    *cell = e;

    SLLQueuePush(m->symbols.first, m->symbols.last, e);

    m->symbols.count += 1;
}

void sea_add_function_symbol_(SeaModule *m, SeaFunctionProto proto) {
    SeaSymbolEntry *e = push_item(m->symbols.arena, SeaSymbolEntry);
    e->name = proto.name;
    e->fn_proto = proto;
    e->kind = SeaSymbolKind_Function;

    sea_add_symbol(m, e);
}

void sea_add_function_symbol(SeaModule *m, SeaFunctionProto proto) {
    rw_mutex_take(m->lock, 1);

    sea_add_function_symbol_(m, proto);

    rw_mutex_drop(m->lock, 1);
}



SeaFunctionGraph *sea_add_function(SeaModule *m, SeaFunctionProto proto) {
    Arena *arena = arena_alloc(.reserve_size = MB(1)); // TODO heuristic for how much

    // This code is ugly as fuck;

    // Yippee
    SeaFunctionGraphNode *fn_node = push_item(arena, SeaFunctionGraphNode);
    SeaFunctionGraph *fn = &fn_node->fn;

    // Set intenals
    {
        fn->proto = proto;
        fn->m = m;
        fn->arena = arena;
        fn->mscope.default_cap = 61;
        fn->map = sea_map_init(arena, 101);

        sea_lattice_init(fn);
        sea_push_new_scope(fn);
        fn->start = sea_create_start(fn, proto);
        fn->stop = sea_create_stop(fn, 4);
    }


    // add the function to the module list
    {
        rw_mutex_take(m->lock, 1);

        SLLQueuePush(m->functions.first, m->functions.last, fn_node);
        m->functions.count += 1;

        sea_add_function_symbol_(m, proto);

        rw_mutex_drop(m->lock, 1);
    }

    return fn;
}
