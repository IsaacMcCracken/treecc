#include "node.h"
#include "x64/encode.c"

void cgx64_naive_expr(X64Emitter *e, SoupNode *expr, X64GPRegister lhs, X64GPRegister rhs){
    switch (expr->kind) {
        case SoupNodeKind_AddI:
        case SoupNodeKind_ConstInt:
        // case SoupNodeKind_AddI:
    }
}

void cgx64_naive_return(X64Emitter *e, SoupNode *ret){
    cgx64_naive_expr(e, ret->inputs[1], X64GPRegister_RAX, X64GPRegister_RCX);
    x64_encode_ret(e);
}
