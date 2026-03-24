#include "sea_internal.h"


SeaEmitter emitter_init(Arena *arena) {
    U8 *code = arena_pos_ptr(arena);
    return (SeaEmitter){
        .arena = arena,
        .code = code,
    };
}

void emitter_push_bytes(SeaEmitter *e, U8 *bytes, U64 len) {
    U8 *data = push_array(e->arena, U8, len);
    MemoryCopy(data, bytes, len);
    e->len += len;
}

void emitter_push_byte(SeaEmitter *e, U8 b) {
    U8 *data = push_item(e->arena, U8);
    *data = b;
    e->len += 1;
}

void emitter_push_s32(SeaEmitter *e, S32 x) {
    U8 buf[4];
    buf[0] = x & 0xFF;
    buf[1] = (x >> 8) & 0xFF;
    buf[2] = (x >> 16) & 0xFF;
    buf[3] = (x >> 24) & 0xFF;
    emitter_push_bytes(e, buf, 4);
}

void emitter_push_s64(SeaEmitter *e, S64 x) {
    U8 buf[8];
    buf[0] = x & 0xFF;
    buf[1] = (x >> 8) & 0xFF;
    buf[2] = (x >> 16) & 0xFF;
    buf[3] = (x >> 24) & 0xFF;
    buf[4] = (x >> 32) & 0xFF;
    buf[5] = (x >> 40) & 0xFF;
    buf[6] = (x >> 48) & 0xFF;
    buf[7] = (x >> 56) & 0xFF;
    emitter_push_bytes(e, buf, 8);
}
