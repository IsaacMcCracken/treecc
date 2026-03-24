#include "sea_x64.h"


typedef U8 X64Mod;
enum {
    X64Mod_NoDisp = 0b00,
    X64Mod_Disp8 = 0b01,
    X64Mod_Disp32 = 0b10,
    X64Mod_Reg = 0b11,
};



void x64_emit_rex_prefix(SeaEmitter *e, X64Reg reg, X64Reg rm, B32 wide) {
    if (reg < 8 && rm < 8 && !wide) return;
    U8 rex = 0x40;
    if (wide) rex |= 0x08;
    if (reg > 0x7) rex |= 0x04;
    if (rm > 0x7) rex |= 0x01;
    emitter_push_byte(e, rex);
}

void x64_mod_reg_rm(SeaEmitter *e, U8 mod, X64Reg reg, X64Reg rm) {
    U8 b = ((mod & 0x3) << 6) | ((reg & 0x7) << 3) | (rm & 0x7);
    emitter_push_byte(e, b);
}

void x64_encode_mov_reg(SeaEmitter *e, X64Reg dst, X64Reg src) {
    x64_emit_rex_prefix(e, src, dst, 1);
    emitter_push_byte(e, 0x89);
    x64_mod_reg_rm(e, X64Mod_Reg, src, dst);
}

void x64_encode_mov_imm(SeaEmitter *e, X64Reg reg, S64 imm) {
    if (imm >= min_S32 && imm <= max_S32) {
        x64_emit_rex_prefix(e, 0, reg, 1);
        emitter_push_byte(e, 0xC7);
        x64_mod_reg_rm(e, X64Mod_Reg, 0, reg);
        S32 imm32 = (S32)imm;
        emitter_push_s32(e, imm32);
    } else {
        x64_emit_rex_prefix(e, 0, 0, 1);
        emitter_push_byte(e, 0xB8 + reg);
        emitter_push_s64(e, imm);
    }
}

void x64_encode_xor(SeaEmitter *e, X64Reg a, X64Reg b) {
    x64_emit_rex_prefix(e, b, a, 1);
    emitter_push_byte(e, 0x31);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}

void x64_encode_add(SeaEmitter *e, X64Reg a, X64Reg b) {
    x64_emit_rex_prefix(e, b, a, 1);
    emitter_push_byte(e, 0x01);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}


void x64_encode_add_imm(SeaEmitter *e, X64Reg reg, S32 imm) {
    x64_emit_rex_prefix(e, 0, reg, 1);
    if (imm < 128 && imm >= -128) {
        emitter_push_byte(e, 0x83);
        x64_mod_reg_rm(e, X64Mod_Reg, 0, reg);

        S8 imm8 = (S8)imm;
        emitter_push_byte(e, *(U8*)&imm8); // bit hacking :)

    } else {
        emitter_push_byte(e, 0x81);
        x64_mod_reg_rm(e, X64Mod_Reg, 0, reg);
        emitter_push_s32(e, imm);
    }
}

void x64_encode_sub(SeaEmitter *e, X64Reg a, X64Reg b) {
    x64_emit_rex_prefix(e, b, a, 1);
    emitter_push_byte(e, 0x29);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}

void x64_encode_sub_imm(SeaEmitter *e, X64Reg reg, S32 imm) {
    x64_emit_rex_prefix(e, 0, reg, 1);
    if (imm < 128 && imm >= -128) {
        emitter_push_byte(e, 0x83);
        x64_mod_reg_rm(e, X64Mod_Reg, 5, reg);
        S8 imm8 = (S8)imm;
        emitter_push_byte(e, *(U8*)&imm8);
    } else {
        emitter_push_byte(e, 0x81);
        x64_mod_reg_rm(e, X64Mod_Reg, 5, reg);
        emitter_push_s32(e, imm);
    }
}

void x64_encode_imul(SeaEmitter *e, X64Reg a, X64Reg b) {
    x64_emit_rex_prefix(e, b, a, 1);
    emitter_push_byte(e, 0x0F);
    emitter_push_byte(e, 0xAF);
    x64_mod_reg_rm(e, X64Mod_Reg, b, a);
}

void x64_encode_imul_imm(SeaEmitter *e, X64Reg a, X64Reg b, S32 imm) {
    x64_emit_rex_prefix(e, b, a, 1);
    if (imm < 128 && imm >= -128) {
        emitter_push_byte(e, 0x6B);
        x64_mod_reg_rm(e, X64Mod_Reg, b, a);

        S8 imm8 = (S8)imm;
        emitter_push_byte(e, *(U8*)&imm8); // bit hacking :)

    } else {
        emitter_push_byte(e, 0x69);
        x64_mod_reg_rm(e, X64Mod_Reg, b, a);
        emitter_push_s32(e, imm);
    }
}

void x64_encode_syscall(SeaEmitter *e) {
    U8 b[2] = {0x0F, 0x05};
    emitter_push_bytes(e, b, 2);
}

void x64_encode_near_jmp(SeaEmitter *e, S32 offset) {
    // this may be causing issues bug we will see
    if ((offset <= -128 && offset <= 0) || (offset < 128 && offset > 0)) {
        emitter_push_byte(e, 0xEB);
        emitter_push_byte(e, (U8)(offset & 0xFF));
    } else {
        emitter_push_byte(e, 0xE9);
        emitter_push_s32(e, offset);
    }

}

void x64_encode_ret(SeaEmitter *e) {
    emitter_push_byte(e, 0xC3);
}

static B32 is_2addr(SeaNode *n) {
    switch (n->kind) {
        case X64Node_Add:
        case X64Node_AddI:
        case X64Node_Sub:
        case X64Node_SubI:
        case X64Node_Mul:
        case X64Node_MulI:
            return 1;
        default:
            return 0;
    }
}

S64 x64_encode(SeaEmitter *e, SeaFunctionGraph *fn, SeaNode *n) {
    U8 node_colour = sea_get_reg_colour(fn, n);

    if (is_2addr(n)) {
        U8 in_colour = sea_get_reg_colour(fn, n->inputs[1]);
        if (node_colour != in_colour) {
            x64_encode_mov_reg(e, node_colour, in_colour);
        }
    }

    switch (n->kind) {
        case X64Node_AddI: {
            x64_encode_add_imm(e, node_colour, (S32)n->vint);
        } break;
        case X64Node_Add: {
            U8 in_colour = sea_get_reg_colour(fn, n->inputs[2]);
            x64_encode_add(e, node_colour, in_colour);
        } break;
        case X64Node_SubI: {
            x64_encode_sub_imm(e, node_colour, (S32)n->vint);
        } break;
        case X64Node_Sub: {
            U8 in_colour = sea_get_reg_colour(fn, n->inputs[2]);
            x64_encode_sub(e, node_colour, in_colour);
        } break;
        case X64Node_MulI: {
            x64_encode_imul_imm(e, node_colour, node_colour, (S32)n->vint);
        } break;
        case X64Node_Mul: {
            U8 in_colour = sea_get_reg_colour(fn, n->inputs[2]);
            x64_encode_imul(e, node_colour, in_colour);
        } break;
        case X64Node_Div: {
            // dividend in RDX:RAX, divisor is inputs[3]
            U8 divisor_colour = sea_get_reg_colour(fn, n->inputs[3]);
            // zero RDX for unsigned div
            x64_encode_xor(e, X64Reg_RDX, X64Reg_RDX);
            x64_emit_rex_prefix(e, 0, divisor_colour, 1);
            emitter_push_byte(e, 0xF7);
            x64_mod_reg_rm(e, X64Mod_Reg, 6, divisor_colour);
        } break;
        case X64Node_Ret: {
            U8 in_colour = sea_get_reg_colour(fn, n->inputs[1]);
            if (in_colour != X64Reg_RAX) {
                x64_encode_mov_reg(e, X64Reg_RAX, in_colour);
            }
            x64_encode_ret(e);
        } break;
        case SeaNodeKind_Copy: {
            U8 in_colour = sea_get_reg_colour(fn, n->inputs[1]);
            if (node_colour != in_colour) {
                x64_encode_mov_reg(e, node_colour, in_colour);
            }
        } break;
        default: break;
    }

    return -1;
}
