#include "sea_internal.h"


String8 sea_node_instr_label(Arena *temp, SeaNode *node) {
    switch (node->kind) {
        case SeaNodeKind_Scope:        return str8_lit("scope??");

        case SeaNodeKind_Return:       return str8_lit("return");
        case SeaNodeKind_Start:        return str8_lit("start");
        case SeaNodeKind_Stop:         return str8_lit("stop");
        case SeaNodeKind_Dead:         return str8_lit("dead");
        case SeaNodeKind_If:           return str8_lit("if");
        case SeaNodeKind_Region:       return str8_lit("region");
        case SeaNodeKind_Loop:         return str8_lit("loop");
        // Integer Arithmetic
        case SeaNodeKind_AddI:         return str8_lit("add");
        case SeaNodeKind_SubI:         return str8_lit("sub");
        case SeaNodeKind_NegI:         return str8_lit("neg");
        case SeaNodeKind_MulI:         return str8_lit("mul");
        case SeaNodeKind_DivI:         return str8_lit("div");
        case SeaNodeKind_ModI:         return str8_lit("mod");
        // Logic
        case SeaNodeKind_Not:          return str8_lit("notl");
        case SeaNodeKind_And:          return str8_lit("andl");
        case SeaNodeKind_Or:           return str8_lit("orl");
        case SeaNodeKind_EqualI:       return str8_lit("eql");
        case SeaNodeKind_NotEqualI:    return str8_lit("neql");
        case SeaNodeKind_GreaterThanI: return str8_lit("gt");
        case SeaNodeKind_GreaterEqualI:return str8_lit("gte");
        case SeaNodeKind_LesserThanI:  return str8_lit("lt");
        case SeaNodeKind_LesserEqualI: return str8_lit("lte");
        // Bitwise
        case SeaNodeKind_BitNotI:      return str8_lit("not");
        case SeaNodeKind_BitAndI:      return str8_lit("and");
        case SeaNodeKind_BitOrI:       return str8_lit("or");
        case SeaNodeKind_BitXorI:      return str8_lit("xor");
        // Data
        case SeaNodeKind_ConstInt:     return str8f(temp, "int %d", (int)node->vint);
        case SeaNodeKind_Proj: {
            if (node->inputs[0]->kind == SeaNodeKind_If) {
                if (!node->vint) return str8_lit("false");
                else return str8_lit("true");
            } else {

            }
        }
        case SeaNodeKind_Phi:          return str8_lit("phi");
    }

    return str8f(temp, "unknown(%d)", (int)node->kind);
}


void dumb_print_bb(SeaNodeMap *m, SeaNode *bb) {
    SeaNode *lookup = sea_map_lookup(m, bb);
    if (lookup) return;

    sea_map_insert(m, bb);

    if (node_is_blockhead(bb) && bb->kind)
        printf("BB %d:\n", (int)bb->nid);
    switch (bb->kind) {
        case SeaNodeKind_If:
        case SeaNodeKind_Return: {
            Temp scratch = scratch_begin(&m->arena, 1);
            String8 opcode = sea_node_instr_label(scratch.arena, bb);
            printf("%%%d %.*s ", (int)bb->nid, str8_varg(opcode));
            scratch_end(scratch);
            for EachIndexFrom(i, 1, bb->inputlen) {
                printf("%%%d ", bb->inputs[i]->nid);
            }
            printf("\n");
        }
    }


    for EachNode(user_node, SeaUser, bb->users) {
        SeaNode *node = sea_user_val(user_node);
        if (!sea_node_is_cfg(node) && node->inputs[0] == bb) {
            sea_map_insert(m, node);
            Temp scratch = scratch_begin(&m->arena, 1);
            String8 opcode = sea_node_instr_label(scratch.arena, node);
            printf("%%%d %.*s ", (int)node->nid, str8_varg(opcode));
            scratch_end(scratch);
            for EachIndexFrom(i, 1, node->inputlen) {
                printf("%%%d ", node->inputs[i]->nid);
            }
            printf("\n");
        }
    }


    for EachNode(user_node, SeaUser, bb->users) {
        SeaNode *node = sea_user_val(user_node);
        if (sea_node_is_cfg(node)) {
            dumb_print_bb(m, node);
        }
    }
}
