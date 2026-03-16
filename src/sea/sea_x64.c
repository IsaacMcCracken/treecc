#include "sea_internal.h"

typedef U16 X64Node;
enum {
    X64Node_AddI = SeaNodeMachStart,
    X64Node_Add,
    X64Node_SubI,
    X64Node_Sub,
    X64Node_MulI,
    X64Node_Mul,
    X64Node_Div,
};

SeaNode *x64_create_add(SeaFunctionGraph *fn, SeaNode *in) {
    SeaNode *lhs = in->inputs[1];
    SeaNode *rhs = in->inputs[2];
    if (rhs->kind == SeaNodeKind_ConstInt) {
        SeaNode *n = sea_node_alloc(fn, X64Node_AddI, 2, 2);
        sea_node_set_input(fn, n, in->inputs[0], 0); // control
        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Add, 3, 3);
        sea_node_set_input(fn, n, in->inputs[0], 0); // control
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
        sea_node_set_input(fn, n, in->inputs[0], 0);
        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Sub, 3, 3);
        sea_node_set_input(fn, n, in->inputs[0], 0);
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
        sea_node_set_input(fn, n, in->inputs[0], 0);
        sea_node_set_input(fn, n, lhs, 1);
        n->vint = rhs->vint;
        return n;
    } else {
        SeaNode *n = sea_node_alloc(fn, X64Node_Mul, 3, 3);
        sea_node_set_input(fn, n, in->inputs[0], 0);
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
    sea_node_set_input(fn, n, in->inputs[0], 0);
    sea_node_set_input(fn, n, lhs, 1);
    sea_node_set_input(fn, n, rhs, 2);

    return n;
}

// Top-level selector: walks a generic IR node and emits x64 machine nodes
SeaNode *x64_select(SeaFunctionGraph *fn, SeaNode *in) {
    switch (in->kind) {
        case SeaNodeKind_Add: return x64_create_add(fn, in);
        case SeaNodeKind_Sub: return x64_create_sub(fn, in);
        case SeaNodeKind_Mul: return x64_create_mul(fn, in);
        case SeaNodeKind_Div: return x64_create_div(fn, in);
        default: in; // pass through (control, const, etc.)
    }
}
