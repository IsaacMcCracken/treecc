// #include "sea.h"
#include "sea_internal.h"


#define LATTICE_CELL_COUNT 101

SeaType sea_type_Top = {.kind = SeaLatticeKind_Top};
SeaType sea_type_Bot = {.kind = SeaLatticeKind_Bot};
SeaType sea_type_CtrlLive = {.kind = SeaLatticeKind_CtrlLive};
SeaType sea_type_CtrlDead = {.kind = SeaLatticeKind_CtrlDead};
SeaType sea_type_S64 = {.kind = SeaLatticeKind_Int, .i = {.min = min_S64, .max = max_S64}};
SeaType sea_type_S64Top = {.kind = SeaLatticeKind_IntTop, .i = {.min = min_S64, . max = max_S64}};
SeaType sea_type_Bool = {.kind = SeaLatticeKind_Int, .i = {.min = 0, .max = 1}};

// If Tuple Data
SeaType *_ifbothdata[2] = {&sea_type_CtrlLive, &sea_type_CtrlLive};
SeaType *_ifnethdata[2] = {&sea_type_CtrlDead, &sea_type_CtrlDead};
SeaType *_iftruedata[2] = {&sea_type_CtrlDead, &sea_type_CtrlLive};
SeaType *_iffalsedata[2] = {&sea_type_CtrlLive, &sea_type_CtrlDead};


SeaType sea_type_IfBoth = {
    .kind = SeaLatticeKind_Tuple,
    .tup = {
        .elems = _ifbothdata,
        .count = 2,
    }
};

SeaType sea_type_IfNeth = {
    .kind = SeaLatticeKind_Tuple,
    .tup = {
        .elems = _ifnethdata,
        .count = 2,
    }
};

SeaType sea_type_IfTrue = {
    .kind = SeaLatticeKind_Tuple,
    .tup = {
        .elems = _iftruedata,
        .count = 2,
    }
};

SeaType sea_type_IfFalse = {
    .kind = SeaLatticeKind_Tuple,
    .tup = {
        .elems = _iffalsedata,
        .count = 2,
    }
};

U64 sea_type_hash(SeaType *t) {
    U64 h = 0xcbf29ce484222325ULL;

    // mix in kind
    h ^= (U64)t->kind;
    h *= 0x00000100000001b3ULL;

    switch (t->kind) {
        case SeaLatticeKind_Top:
        case SeaLatticeKind_Bot:
        case SeaLatticeKind_CtrlLive:
        case SeaLatticeKind_CtrlDead:
        case SeaLatticeKind_SIMPLE: {
            // kind is the whole identity
        } break;

        case SeaLatticeKind_Int: {
            h ^= (U64)t->i.min;
            h *= 0x00000100000001b3ULL;
            h ^= (U64)t->i.max;
            h *= 0x00000100000001b3ULL;
        } break;

        case SeaLatticeKind_Tuple: {
            h ^= t->tup.count;
            h *= 0x00000100000001b3ULL;
            for EachIndex(i, t->tup.count) {
                h ^= (U64)t->tup.elems[i]; // pointer identity, types are interned
                h *= 0x00000100000001b3ULL;
            }
        } break;

        case SeaLatticeKind_Struct: {
            U8 *buf = (U8*)t->s.name.str;
            for (U64 i = 0; i < t->s.name.size; i++) {
                h ^= buf[i];
                h *= 0x00000100000001b3ULL;
            }
        } break;
    }

    // avalanche
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    return h;
}

void sea_type_insert_raw(SeaModule *m, SeaType *t) {
    U64 idx = sea_type_hash(t) % m->lat->cap;
    SeaType **cell = &m->lat->cells[idx];
    while (*cell) {
        *cell = (*cell)->hash_next;
    }
    *cell = t;
}


void sea_lattice_init(SeaModule *m) {
    // Alloc and set up
    {
        Arena *arena = arena_alloc();
        SeaTypeLattice *lat = push_item(arena, SeaTypeLattice);
        lat->cells =  push_array(arena, SeaType*, LATTICE_CELL_COUNT);
        lat->cap = LATTICE_CELL_COUNT;
        lat->arena = arena;
        lat->lock = rw_mutex_alloc();
        m->lat = lat;
    }



    sea_type_insert_raw(m, &sea_type_Top);
    sea_type_insert_raw(m, &sea_type_Bot);
    sea_type_insert_raw(m, &sea_type_CtrlLive);
    sea_type_insert_raw(m, &sea_type_CtrlDead);
    sea_type_insert_raw(m, &sea_type_S64);
    sea_type_insert_raw(m, &sea_type_S64Top);
    sea_type_insert_raw(m, &sea_type_Bool);
    sea_type_insert_raw(m, &sea_type_IfBoth);
    sea_type_insert_raw(m, &sea_type_IfNeth);
    sea_type_insert_raw(m, &sea_type_IfTrue);
    sea_type_insert_raw(m, &sea_type_IfFalse);

}

B32 sea_type_equal(SeaType *a, SeaType *b) {
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
        case SeaLatticeKind_Top:
        case SeaLatticeKind_Bot:
        case SeaLatticeKind_IntTop:
        case SeaLatticeKind_CtrlLive:
        case SeaLatticeKind_CtrlDead: {
            return 1; // kind is the whole identity
        } break;
        case SeaLatticeKind_Int: {
            return a->i.min == b->i.min && a->i.max == b->i.max;
        } break;
        case SeaLatticeKind_Tuple: {
            if (a->tup.count != b->tup.count) return 0;
            return MemoryMatchTyped(a->tup.elems, b->tup.elems, a->tup.count);
        } break;
        case SeaLatticeKind_Struct: {
            if (!str8_match(a->s.name, b->s.name, 0)) return 0;
            if (a->s.fields.count != b->s.fields.count) return 0;
            for EachIndex(i, a->s.fields.count) {
                SeaField *fa = &a->s.fields.fields[i];
                SeaField *fb = &b->s.fields.fields[i];
                if (!str8_match(fa->name, fb->name, 0)) return 0;
                if (fa->type != fb->type) return 0; // pointer identity since types are interned
            }
            return 1;
        } break;
    }
    return 0;
}

SeaType *sea_type_canonical(SeaModule *m, SeaType *t) {
    SeaTypeLattice *lat = m->lat;

    rw_mutex_take_w(lat->lock);
    U64 idx = sea_type_hash(t) % lat->cap;
    SeaType **cell = &lat->cells[idx];
    while (*cell) {
        if (sea_type_equal(*cell, t)) {
            return *cell;
        }
        *cell = (*cell)->hash_next;
    }

    SeaType *canon = push_item(lat->arena, SeaType);

    *canon = *t;
    canon->hash_next = 0;
    *cell = canon;

    rw_mutex_drop_w(lat->lock);

    return canon;
}

SeaType *sea_type_xmeet(SeaFunctionGraph *fn, SeaType *a, SeaType *b) {
    if (a->kind < SeaLatticeKind_SIMPLE || b->kind < SeaLatticeKind_SIMPLE) {
        if (a == &sea_type_Bot || b == &sea_type_Top) return a;
        if (b == &sea_type_Bot || a == &sea_type_Top) return b;
        if (b->kind > SeaLatticeKind_SIMPLE)
            return &sea_type_Bot;
        if (a == &sea_type_CtrlLive || b == &sea_type_CtrlLive)
            return &sea_type_CtrlLive;
        return &sea_type_CtrlDead;
    }

    switch (a->kind) {
        case SeaLatticeKind_Int: {
            // For better optimizations we could do multirange integers
            // but that sounds too cumbersome so we just take the widest
            // range
            S64 max = Max(a->i.max, b->i.max);
            S64 min = Min(a->i.min, b->i.min);
            SeaType t = {
                .kind = SeaLatticeKind_Int,
                .i = {
                    .min = min,
                    .max = max,
                },
            };

            return sea_type_canonical(fn->m, &t);
        } break;
    }



    return &sea_type_Bot;
}

SeaType *sea_type_meet(SeaFunctionGraph *fn, SeaType *a, SeaType *b) {
    if (a == b) return a;
    if (a->kind == b->kind) return sea_type_xmeet(fn, a, b);
    if (a->kind < SeaLatticeKind_SIMPLE)
        return sea_type_xmeet(fn, a, b);
    if (b->kind < SeaLatticeKind_SIMPLE)
        return sea_type_xmeet(fn, b, a);

    return &sea_type_Bot;
}



B32 sea_type_is_const_int(SeaType *t) {
    switch (t->kind) {
        case SeaLatticeKind_Int: {
            return t->i.min == t->i.max;
        } break;
    }

    return 0;
}

S64 sea_type_const_int_val(SeaType *t) {
    Assert(t->i.min == t->i.max);
    return t->i.min;
}

SeaType *sea_type_tuple(SeaFunctionGraph *fn, SeaType **elems, U64 count) {
    SeaType tup = (SeaType){
        .kind = SeaLatticeKind_Tuple,
        .tup = {
            .elems = elems,
            .count = count,
        }
    };

    return sea_type_canonical(fn->m, &tup);
}

SeaType *sea_type_const_int(SeaFunctionGraph *fn, S64 v) {
    SeaType t = {
        .kind = SeaLatticeKind_Int,
        .i = {
            .min = v,
            .max = v,
        },
    };

    return sea_type_canonical(fn->m, &t);
}


SeaType *sea_type_make_func(SeaModule *m, SeaFunctionProto func) {
    SeaType t = (SeaType) {
        .kind = SeaLatticeKind_Func,
        .func = func,
    };
    return sea_type_canonical(m, &t);
}

 SeaType *sea_type_make_struct(SeaModule *m, SeaTypeStruct s) {
     SeaType t = (SeaType) {
         .kind = SeaLatticeKind_Struct,
         .s = s
     };

     return sea_type_canonical(m, &t);
 }
