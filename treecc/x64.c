#include <soup.h>
#include "x64/encode.c"

void cgx64_naive_expr(X64Emitter *e, SoupNode *expr, X64GPRegister lhs, X64GPRegister rhs){
    swit
}

void cgx64_naive_return(X64Emitter *e, SoupNode *return){
    cgx64_naive_expr(e, return->inputs[1], X64GPRegister_RAX, X64GPRegister_RCX);
    x64_encode_ret(e);
}
