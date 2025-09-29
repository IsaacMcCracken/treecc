/*
typedef struct BufferBuilder BufferBuilder;
struct BufferBuilder {
  Arena *arena,
  Byte *buf;
  U64 len;
};
- [*] `BufferBuilder builder_init(Arena *arena);`
- [ ] `String builder_to_string(BuilderBuilder *b);`
- [ ] `void builder_write_string(BufferBuilder *b, String s);`
- [ ] `void builder_write_float32(BufferBuilder *b, F32 x);`
- [ ] `void builder_write_int(BufferBuilder *b, S32 x);`

And other functions you find fitting. I think this will be a fun exercise, plus it will be useful for generating machine code. Yippee! :) 
*/
#include <core.h>

BufferBuilder builder_init(Arena *arena) {
    Byte *base = arena_get_current(arena);
    return (BufferBuilder){
        .arena = arena,
        .base = base,
        .len = 0,
    };
}

void builder_push_bytes(BufferBuilder *bb, Byte *bytes, U64 len) {
    Byte *data = arena_push_array(bb->arena, Byte, len);
    mem_cpy(data, bytes, len);
    bb->len += len;
}

void builder_push_byte(BufferBuilder *bb, Byte b) {
    Byte *data = arena_push(bb->arena, Byte);
    *data = b;
    bb->len += 1;
}

#define builder_write(/*type*/ T, /*BufferBuilder*/ B, V /*type_ref*/) \
    builder_push_bytes(B, (Byte *)type_ref, sizeof(T))

String builder_to_string(BufferBuilder *bb) {
    return (String){ .str = (char *)bb->base, .len = bb->len };
}

void builder_write_number(BufferBuilder *bb, U64 num, const U8 base) {
    const char *conversion = "0123456789abcdef";
    U64 start = bb->len; //get the current cursor position
    if (num == 0) {
        builder_push_byte(bb, (Byte)'0');
        return;
    }
    while (num != 0) {
        builder_push_byte(bb, conversion[(num % base)]);
        num /= base;
    }
    char temp;
    U64 end = bb->len - 1;
    for (; start < end; start++, end--) {
        temp = bb->base[start];
        bb->base[start] = bb->base[end];
        bb->base[end] = temp;
    }
}

void builder_write_signed_dec(BufferBuilder *bb, S64 num) {
    if (num < 0) {
        builder_push_byte(bb, (Byte)'-');
        num = -num;
    }
    builder_write_number(bb, (U64)num, 10);
}
