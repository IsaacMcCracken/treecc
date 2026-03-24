#ifndef SEA_X64_H
#define SEA_X64_H

#include "sea_internal.h"


#define SYSV_CONV (CallConv){ \
    .args = {                 \
        X64Reg_RDI,           \
        X64Reg_RSI,           \
        X64Reg_RDX,           \
        X64Reg_RCX,           \
        X64Reg_R8,            \
        X64Reg_R9,            \
    },                        \
    .arglen = 6,              \
}

typedef U16 X64Node;
enum {
    // Arithmetic
    X64Node_AddI = 0x100,
    X64Node_Add,
    X64Node_SubI,
    X64Node_Sub,
    X64Node_MulI,
    X64Node_Mul,
    X64Node_Div,
    // Control Flow
    X64Node_Ret,
    X64Node_Jmp,
    X64Node_Set,
    X64Node_CmpI,
    X64Node_Cmp,
};

typedef U8 X64Reg;
enum {
    X64Reg_RAX,
    X64Reg_RCX,
    X64Reg_RDX,
    X64Reg_RBX,
    X64Reg_RSP,
    X64Reg_RBP,
    X64Reg_RSI,
    X64Reg_RDI,
    X64Reg_R8,
    X64Reg_R9,
    X64Reg_R10,
    X64Reg_R11,
    X64Reg_R12,
    X64Reg_R13,
    X64Reg_R14,
    X64Reg_R15,
};

const char *x64_reg_name(X64Reg r);
S64 x64_encode(SeaEmitter *e, SeaFunctionGraph *fn, SeaNode *n);

#endif // SEA_X64_H
