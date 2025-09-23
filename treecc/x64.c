#include "node.h"
#include "x64/encode.c"

X64GPRegister args[6] = {X64GPRegister_RDI, X64GPRegister_RSI, X64GPRegister_RCX, X64GPRegister_RDX, X64GPRegister_R8, X64GPRegister_R9};

X64GPRegister cgx64_naive_expr(X64Emiter *e, TreeNode *expr){
    TreeNode *lhs = expr->inputs[0];
    TreeNode *rhs = expr->inputs[1];
    switch (expr->kind) {
        case TreeNodeKind_AddI: {

        } break;
        case TreeNodeKind_MulI: {
            X64GPRegister l = cgx64_naive_expr(e, lhs);
            if (rhs->kind == TreeNodeKind_ConstInt) {
                x64_encode_imul_imm(e, l, l, rhs->vint);
            } else {
                X64GPRegister r = cgx64_naive_expr(e, rhs);
                x64_encode_imul(e, l, r);
            }
            return l;
        } break;
        case TreeNodeKind_ConstInt: {

        } break;
        case TreeNodeKind_Proj: {
            return args[expr->vint];
        }
    }

    return 0;
}

void cgx64_naive_return(X64Emiter *e, TreeNode *ret){
    X64GPRegister out = cgx64_naive_expr(e, ret->inputs[1]);
    if (out != X64GPRegister_RAX) {
        x64_encode_mov_reg(e, X64GPRegister_RAX, out);
    }
    x64_encode_ret(e);
}
