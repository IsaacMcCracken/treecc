#include "node.h"
#include "x64/encode.c"

X64GPRegister args[6] = {X64GPRegister_RDI, X64GPRegister_RSI, X64GPRegister_RCX, X64GPRegister_RDX, X64GPRegister_R8, X64GPRegister_R9};

X64GPRegister cgx64_naive_expr(X64Emiter *e, TreeNode *expr, X64GPRegister lhs, X64GPRegister rhs){
    switch (expr->kind) {
        case TreeNodeKind_AddI: {

        } break;
        case TreeNodeKind_MulI: {
            X64GPRegister l = cgx64_naive_expr(e, expr->inputs[0], lhs, rhs);
            X64GPRegister r = cgx64_naive_expr(e, expr->inputs[1], lhs, rhs);
            x64_encode_imul(e, l, r);
            return l;
        } break;
        case TreeNodeKind_ConstInt: {

        } break;
        case TreeNodeKind_Proj: {
            return args[expr->vint];
        }
    }

    return lhs;
}

void cgx64_naive_return(X64Emiter *e, TreeNode *ret){
    X64GPRegister out = cgx64_naive_expr(e, ret->inputs[1], X64GPRegister_RAX, X64GPRegister_RCX);
    if (out != X64GPRegister_RAX) {
        x64_encode_mov_reg(e, X64GPRegister_RAX, out);
    }
    x64_encode_ret(e);
}
