#include <core.h>

typedef struct X64Emiter X64Emiter;
struct X64Emiter {
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
    X64Mod_Disp32 = 0b10,
    X64Mod_Reg = 0b11,
};

X64Emiter x64_emiter_init(Arena *arena) {
    Byte *code = arena_get_current(arena);
    return (X64Emiter){
        .arena = arena,
        .code = code,
    };
}

void x64_emiter_push_bytes(X64Emiter *e, Byte *bytes, U64 len) {
    Byte *data = arena_push_array(e->arena, Byte, len);
    mem_cpy(data, bytes, len);
    e->len += len;
}

void x64_emiter_push_byte(X64Emiter *e, Byte b) {
    Byte *data = arena_push(e->arena, Byte);
    *data = b;
    e->len += 1;
}

void x64_emit_s32(X64Emiter *e, S32 x) {
    Byte buf[4];
    buf[0] = x & 0xFF;
    buf[1] = (x >> 8) & 0xFF;
    buf[2] = (x >> 16) & 0xFF;
    buf[3] = (x >> 24) & 0xFF;
    x64_emiter_push_bytes(e, buf, 4);
}

void x64_emit_rex_prefix(
    X64Emiter *e,
    X64GPRegister reg,
    X64GPRegister rm,
    B32 wide
) {
    if (reg < 8 && rm < 8 && !wide) return;
    Byte rex = 0x40;
    if (wide) rex |= 0x08;
    if (reg > 0x7) rex |= 0x04;
    if (rm > 0x7) rex |= 0x01;
    x64_emiter_push_byte(e, rex);
}

void x64_mod_reg_rm(X64Emiter *e, U8 mod, X64GPRegister reg, X64GPRegister rm) {
    Byte b = ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7);
    x64_emiter_push_byte(e, b);
}

void x64_encode_add(X64Emiter *e, X64GPRegister a, X64GPRegister b) {
    x64_emit_rex_prefix(e, b, a, 1);
    x64_emiter_push_byte(e, 0x01);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}

void x64_encode_sub(X64Emiter *e, X64GPRegister a, X64GPRegister b) {
    x64_emit_rex_prefix(e, b, a, 1);
    x64_emiter_push_byte(e, 0x29);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}

void x64_encode_syscall(X64Emiter *e) {
    Byte b[2] = {0x0F, 0x05};
    x64_emiter_push_bytes(e, b, 2);
}

void x64_encode_near_jmp(X64Emiter *e, S32 offset) {
    // TODO check if offset can be an S8;

    x64_emiter_push_byte(e, 0xE9);
    x64_emit_s32(e, offset);
}

void x64_encode_ret(X64Emiter *e) {
    x64_emiter_push_byte(e, 0xC3);
}
