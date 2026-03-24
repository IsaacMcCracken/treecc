#include "sea_x64.h"

#define X64_RMASK rmask_u64(0x0F) // all GPR int regs
#define X64_RET_MASK rmask_u64(1 << X64Reg_RAX)
#define X64_NONE_MASK rmask_u64(0);


const char *x64_reg_name(X64Reg r) {
    switch (r) {
        case X64Reg_RAX: return "rax";
        case X64Reg_RCX: return "rcx";
        case X64Reg_RDX: return "rdx";
        case X64Reg_RBX: return "rbx";
        case X64Reg_RSP: return "rsp";
        case X64Reg_RBP: return "rbp";
        case X64Reg_RSI: return "rsi";
        case X64Reg_RDI: return "rdi";
        case X64Reg_R8:  return "r8";
        case X64Reg_R9:  return "r9";
        case X64Reg_R10: return "r10";
        case X64Reg_R11: return "r11";
        case X64Reg_R12: return "r12";
        case X64Reg_R13: return "r13";
        case X64Reg_R14: return "r14";
        case X64Reg_R15: return "r15";
    }

    return "wtf";
}

B32 mach_node_is_cfg(SeaNode *n) {
    switch (n->kind) {
        case X64Node_Ret:
        case X64Node_Jmp:
            return 1;
    }
    return 0;
}


SeaNode *x64_create_add(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *lhs = in->inputs[1];
    SeaNode *rhs = in->inputs[2];
    if (rhs->kind == SeaNodeKind_ConstInt) {
        SeaNode *n = sea_node_alloc(fn, X64Node_AddI, 2, 2);
        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Add, 3, 3);
         // control
        sea_node_set_input(fn, n, lhs, 1);
        sea_node_set_input(fn, n, rhs, 2);
        return n;
    }
}

SeaNode *x64_create_sub(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *lhs = in->inputs[1];
    SeaNode *rhs = in->inputs[2];
    if (rhs->kind == SeaNodeKind_ConstInt) {
        SeaNode *n = sea_node_alloc(fn, X64Node_SubI, 2, 2);

        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Sub, 3, 3);

        sea_node_set_input(fn, n, lhs, 1);
        sea_node_set_input(fn, n, rhs, 2);
        return n;
    }
}

SeaNode *x64_create_mul(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *lhs = in->inputs[1];
    SeaNode *rhs = in->inputs[2];
    if (rhs->kind == SeaNodeKind_ConstInt) {
        SeaNode *n = sea_node_alloc(fn, X64Node_MulI, 2, 2);

        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Mul, 3, 3);

        sea_node_set_input(fn, n, lhs, 1);
        sea_node_set_input(fn, n, rhs, 2);
        return n;
    }
}

// Div has no immediate form - x64 IDIV always uses a register
SeaNode *x64_create_div(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *lhs = in->inputs[1];
    SeaNode *rhs = in->inputs[2];
    SeaNode *n = sea_node_alloc(fn, X64Node_Div, 3, 3);

    sea_node_set_input(fn, n, lhs, 1);
    sea_node_set_input(fn, n, rhs, 2);

    return n;
}


SeaNode *x64_create_cmp(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *n = sea_node_alloc(fn, X64Node_Set, 2, 2);
    sea_node_set_input(fn, n, in->inputs[1], 1);
    return n;
}

SeaNode *x64_create_jmp(SeaFunctionGraph *fn, SeaNode *ifnode) {
    SeaNode *n = sea_node_alloc(fn, X64Node_Jmp, 2, 2);
    SeaNode *ctrl = ifnode->inputs[0];
    SeaNode *cond = ifnode->inputs[1];
    // if (!sea_node_is_bool(cond)) {
    //     SeaNode *zero = sea_create_const_int(fn, 0);
    //     cond = sea_create_bin_op(fn, SeaNodeKind_EqualI, zero, cond);
    //     sea_node_set_input(fn, ifnode, cond, 1);
    // }
    // vint is for loop depth
    sea_node_set_input(fn, n, ctrl, 0);
    sea_node_set_input(fn, n, cond, 1);
    return n;
}

SeaNode *x64_create_ret(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *n = sea_node_alloc(fn, X64Node_Ret, 2, 2);
    sea_node_set_input(fn, n, in->inputs[0], 0);
    sea_node_set_input(fn, n, in->inputs[1], 1);
    return n;
}

SeaNode *mach_create_clone(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *n = sea_node_alloc(fn, in->kind, in->inputcap, in->inputlen);
    n->type = in->type;
    n->vint = in->vint;
    for EachIndex(i, in->inputlen) {
        sea_node_set_input(fn, n, in->inputs[i], i);
    }
    return n;
}

// Top-level selector: walks a generic IR node and emits x64 machine nodes
SeaNode *x64_select(SeaFunctionGraph *fn, SeaNode *in) {
    switch (in->kind) {
        case SeaNodeKind_AddI: return x64_create_add(fn, in);
        case SeaNodeKind_SubI: return x64_create_sub(fn, in);
        case SeaNodeKind_MulI: return x64_create_mul(fn, in);
        case SeaNodeKind_DivI: return x64_create_div(fn, in);
        // case SeaNodeKind_Not:

        case SeaNodeKind_If:            return x64_create_jmp(fn, in);
        case SeaNodeKind_Return:        return x64_create_ret(fn, in);

        case SeaNodeKind_And:
        case SeaNodeKind_Or:
        case SeaNodeKind_EqualI:
        case SeaNodeKind_NotEqualI:
        case SeaNodeKind_GreaterThanI:
        case SeaNodeKind_GreaterEqualI:
        case SeaNodeKind_LesserThanI:
        case SeaNodeKind_LesserEqualI:
        case SeaNodeKind_Proj:
        case SeaNodeKind_Phi:
        case SeaNodeKind_Start:
        case SeaNodeKind_Stop:
        case SeaNodeKind_Loop:
        case SeaNodeKind_Region:        return mach_create_clone(fn, in);
        default: {Trap();} break;
    }
}


RegMask x64_rmask_out(SeaNode *n) {
    switch (n->kind) {
        case SeaNodeKind_Proj: {
            SeaNode *input = n->inputs[0];
            switch (input->kind) {
                case SeaNodeKind_Start: {
                    if (n->vint == 0) return X64_NONE_MASK;
                    U8 idx = n->vint - 1;
                    return rmask_u64(1<<(mach.conv.args[idx]));
                } break;
            }
            return X64_NONE_MASK;
        } break;
        // No output
        case X64Node_Ret:
        case X64Node_Jmp:
        case X64Node_CmpI:
        case X64Node_Cmp:   return X64_NONE_MASK;

        // Fixed output: RAX only
        case X64Node_Div:   return X64_RET_MASK;

        // Any GPR
        case X64Node_AddI:
        case X64Node_Add:
        case X64Node_SubI:
        case X64Node_Sub:
        case X64Node_MulI:
        case X64Node_Mul:
        case X64Node_Set:   return X64_RMASK;

        default: { Trap(); } break;
    }
}

RegMask x64_rmask_in(SeaNode *node, U16 idx) {
    switch (node->kind) {
        case X64Node_Ret:  {
            if (idx == 0) return X64_NONE_MASK
            if (idx == 1) return X64_RET_MASK;  // value must be in RAX
            break;
        }
        case X64Node_Div: {
            if (idx == 0) return X64_NONE_MASK;                  // ctrl
            if (idx == 1) return rmask_u64(1 << X64Reg_RAX);    // rax
            if (idx == 2) return rmask_u64(1 << X64Reg_RDX);    // rdx
            if (idx == 3) return X64_RMASK;                      // divisor
            break;
        }

        case X64Node_Jmp:
        case X64Node_Set:
        case X64Node_AddI:
        case X64Node_SubI:
        case X64Node_MulI: {
            if (idx == 0) return X64_NONE_MASK; // ctrl
            if (idx == 1) return X64_RMASK; // lhs (imm is in vint, not an input)
            break;
        }

        case X64Node_Add:
        case X64Node_Sub:
        case X64Node_Mul:
        case X64Node_Cmp: {
            if (idx == 0) return X64_NONE_MASK; // ctrl
            if (idx == 1) return X64_RMASK;     // lhs
            if (idx == 2) return X64_RMASK;     // rhs
            break;
        }
    }
    Trap();
}

SeaMach mach = (SeaMach){
    .name = (String8){"x64", 3},
    .select = x64_select,
    .rmask_out = x64_rmask_out,
    .rmask_in = x64_rmask_in,
    .encode = x64_encode,
    .conv = SYSV_CONV,
};
