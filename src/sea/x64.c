#include "node.h"
#include "x64/encode.c"

X64GPRegister args[6] = {X64GPRegister_RDI, X64GPRegister_RSI, X64GPRegister_RCX, X64GPRegister_RDX, X64GPRegister_R8, X64GPRegister_R9};

X64GPRegister cgx64_naive_expr(X64Emiter *e, TreeNode *expr, X64GPRegister target){
    switch (expr->kind) {
        case TreeNodeKind_AddI: {
            TreeNode *lhs = expr->inputs[0];
            TreeNode *rhs = expr->inputs[1];
            X64GPRegister l = cgx64_naive_expr(e, lhs, target);
            if (rhs->kind == TreeNodeKind_ConstInt) {
                x64_encode_add_imm(e, l, rhs->vint);
            } else {
                X64GPRegister r = cgx64_naive_expr(e, rhs, target);
                x64_encode_add(e, l, r);
            }
            return l;
        } break;
        case TreeNodeKind_MulI: {
            TreeNode *lhs = expr->inputs[0];
            TreeNode *rhs = expr->inputs[1];
            X64GPRegister l = cgx64_naive_expr(e, lhs, target);
            if (rhs->kind == TreeNodeKind_ConstInt) {
                x64_encode_imul_imm(e, l, l, rhs->vint);
            } else {
                X64GPRegister r = cgx64_naive_expr(e, rhs, target);
                x64_encode_imul(e, l, r);
            }
            return l;
        } break;
        case TreeNodeKind_ConstInt: {
            if (expr->vint == 0) {
                x64_encode_xor(e, target, target);
            } else {
                x64_encode_mov_imm(e, target, expr->vint);
            }
        } break;
        case TreeNodeKind_Proj: {
            return args[expr->vint];
        }
    }

    return target;
}

void cgx64_naive_return(X64Emiter *e, TreeNode *ret){
    X64GPRegister out = cgx64_naive_expr(e, ret->inputs[1], X64GPRegister_RAX);
    if (out != X64GPRegister_RAX) {
        x64_encode_mov_reg(e, X64GPRegister_RAX, out);
    }
    x64_encode_ret(e);
}
