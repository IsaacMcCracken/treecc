#include <core.h>

typedef struct SoupEmiter SoupEmiter;
struct SoupEmiter {
    Arena *arena;
    Byte *code;
    U64 len;
};

typedef U8 X64GPRegister;
enum {
    X64GPRegister_RAX,
    X64GPRegister_RBX,
    X64GPRegister_RCX,
    X64GPRegister_RDX,
    X64GPRegister_RSI,
    X64GPRegister_RDI,
    X64GPRegister_RSP,
    X64GPRegister_RBP,
    X64GPRegister_R8,
    X64GPRegister_R9,
    X64GPRegister_R10,
    X64GPRegister_R11,
    X64GPRegister_R12,
    X64GPRegister_R13,
    X64GPRegister_R14,
    X64GPRegister_R15,
};

typedef U8 X64Mod;
enum {
    X64Mod_NoDisp = 0b00,
    X64Mod_Disp8 = 0b01,
    X64Mod_Disp32 = 0x10,
    X64Mod_Reg = 0b11,
};

SoupEmiter soup_emiter_init(Arena *arena) {
    Byte *code = arena_get_current(arena);
    return (SoupEmiter){
        .arena = arena,
        .code = code,
    };
}

void soup_emiter_push_bytes(SoupEmiter *e, Byte *bytes, U64 len) {
    Byte *data = arena_push_array(e->arena, Byte, len);
    mem_cpy(data, bytes, len);
}

void soup_emiter_push_byte(SoupEmiter *e, Byte b) {
    Byte *data = arena_push(e->arena, Byte);
    *data = b;
}

void soup_x64_emit_rex_prefix(
    SoupEmiter *e,
    X64GPRegister reg,
    X64GPRegister rm,
    B32 wide
) {
    if (reg < 8 && rm < 8 && !wide) return;
    Byte rex = 0x40;
    if (wide) rex |= 0x08;
    if (reg > 0x7) rex |= 0x04;
    if (rm > 0x7) rex |= 0x01;
    soup_emiter_push_byte(e, rex);
}

void soup_x64_mod_reg_rm(SoupEmiter *e, U8 mod, X64GPRegister reg, X64GPRegister rm) {
    Byte b = (mod << 6) | (reg << 3) | rm;
    soup_emiter_push_byte(e, b);
}

void soup_x64_encode_add(SoupEmiter *e, X64GPRegister a, X64GPRegister b) {
    soup_emit_rex_prefix(e, b, a, 1)
    soup_emiter_push_byte(e, 0x01);
    soup_x64_mod_reg_rm(e, )
}
