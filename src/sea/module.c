#include <sea/sea.h>

SeaModule sea_create_module(void) {
    // TODO create symbol table
    Arena *arena = arena_alloc(MB(64));
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
    Arena *arena = arena_alloc(MB(1)); // TODO heuristic for how much

    // Yippee
    SeaFunctionGraphNode *fn_node = push_item(arena, SeaFunctionGraphNode);
    SeaFunctionGraph *fn = &fn_node->fn;
    fn->proto = proto;
    fn->m = m;
    fn->mscope.default_cap = 61;

    fn->map = sea_map_init(arena, 101);

    SeaNode *start = push_item(arena, SeaNode);
    start->kind = SeaNodeKind_Start;
    SeaNode *stop = push_item(arena, SeaNode);
    stop->kind = SeaNodeKind_Stop;

    fn->start = start;
    fn->stop = stop;
    fn->arena = arena;

    sea_push_new_scope(fn);

    for EachIndex(i, proto.args.count) {
        SeaNode *arg_node = sea_create_proj(fn, fn->start, i);
        arg_node->type = proto.args.fields[i].type;
        sea_insert_local_symbol(fn, proto.args.fields[i].name, arg_node);
    }




    {
        rw_mutex_take(m->lock, 1);

        SLLQueuePush(m->functions.first, m->functions.last, fn_node);
        m->functions.count += 1;

        sea_add_function_symbol_(m, proto);

        rw_mutex_drop(m->lock, 1);
    }

    return fn;
}
