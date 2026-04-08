#include "sea_internal.h"

typedef S64 (*JitSimpFn)(S64);

S64 sea_jit(SeaModule *m, S64 x) {
    JitSimpFn fn = 0;
    for EachNode(sym, SeaSymbolEntry, m->symbols.first) {
        if (str8_match(str8_lit("simp"), sym->name, 0)) {
            fn = (JitSimpFn)(&m->emit.code[sym->pos_in_section]);
        }
    }
    os_protect(m->emit.arena, m->emit.arena->cmt_size, OS_AccessFlag_Execute);
    S64 result = fn(x);
    os_protect(m->emit.arena, m->emit.arena->cmt_size, OS_AccessFlag_Read|OS_AccessFlag_Write);
    return result;
}
