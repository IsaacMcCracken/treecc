// #include "sea_internal.h"
#include "sea_x64.h"

#define SEA_SHOW_GRAPH 1

String8 sea_node_label(Arena *temp, SeaNode *node) {
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
        case SeaNodeKind_AddI:         return str8_lit("+");
        case SeaNodeKind_SubI:         return str8_lit("-");
        case SeaNodeKind_NegI:         return str8_lit("neg");
        case SeaNodeKind_MulI:         return str8_lit("*");
        case SeaNodeKind_DivI:         return str8_lit("/");
        case SeaNodeKind_ModI:         return str8_lit("%");
        // Logic
        case SeaNodeKind_Not:          return str8_lit("!");
        case SeaNodeKind_And:          return str8_lit("&&");
        case SeaNodeKind_Or:           return str8_lit("||");
        case SeaNodeKind_EqualI:       return str8_lit("==");
        case SeaNodeKind_NotEqualI:    return str8_lit("!=");
        case SeaNodeKind_GreaterThanI: return str8_lit(">");
        case SeaNodeKind_GreaterEqualI:return str8_lit(">=");
        case SeaNodeKind_LesserThanI:  return str8_lit("<");
        case SeaNodeKind_LesserEqualI: return str8_lit("<=");
        // Bitwise
        case SeaNodeKind_BitNotI:      return str8_lit("~");
        case SeaNodeKind_BitAndI:      return str8_lit("&");
        case SeaNodeKind_BitOrI:       return str8_lit("|");
        case SeaNodeKind_BitXorI:      return str8_lit("^");
        // Data
        case SeaNodeKind_ConstInt:     return str8f(temp, "%d", (int)node->vint);
        case SeaNodeKind_Proj: {
            SeaNode *input = node->inputs[0];
            switch (input->kind) {
                case X64Node_Jmp:
                case SeaNodeKind_If: {
                    if (!node->vint) return str8_lit("false");
                    else return str8_lit("true");
                } break;

                default: {
                    SeaType *type = node->inputs[0]->type->tup.elems[node->vint];

                    switch (type->kind) {
                        case SeaLatticeKind_Int: return str8_lit("int");
                        case SeaLatticeKind_CtrlLive: return str8_lit("ctrl");
                        case SeaLatticeKind_CtrlDead: return str8_lit("xctrl (x_x)");
                    }

                    return str8f(temp, "proj(%d)", (int)node->vint);
                } break;
            }
        }
        case SeaNodeKind_Phi:          return str8_lit("phi");
        case SeaNodeKind_Copy:         return str8_lit("copy");
        // X64
        // X64 Arithmetic
        case X64Node_AddI: return str8f(temp, "add $%lld", node->vint);
        case X64Node_Add:  return str8_lit("add");
        case X64Node_SubI: return str8f(temp, "sub $%lld", node->vint);
        case X64Node_Sub:  return str8_lit("sub");
        case X64Node_MulI: return str8f(temp, "imul $%lld", node->vint);
        case X64Node_Mul:  return str8_lit("imul");
        case X64Node_Div:  return str8_lit("idiv");
        // X64 Control Flow
        case X64Node_Ret:  return str8_lit("ret");
        case X64Node_Jmp:  return str8_lit("jmp");
        case X64Node_Set:  return str8_lit("set");
        case X64Node_CmpI: return str8f(temp, "cmp $%lld", node->vint);
        case X64Node_Cmp:  return str8_lit("cmp");
    }

    return str8f(temp, "unknown(%d)", (int)node->kind);
}



void sea_graphviz_visit_node(FILE *fp, Arena *temp, SeaNodeMap *map, SeaNode *node) {
    SeaNode *lookup = sea_map_lookup(map, node);
    if (lookup) return;

    sea_map_insert(map, node);


    for EachIndex(i, node->inputlen) {
        SeaNode *input = node->inputs[i];
        if (input) {
            sea_graphviz_visit_node(fp, temp, map, input);
            if (i != 0)
                fprintf(fp, "n%p -> n%p;\n", input, node);
            else
                fprintf(fp, "n%p -> n%p [color=\"green\"];\n", input, node);

        }
    }


    for EachNode(user_node, SeaUser, node->users) {
        SeaNode *user = sea_user_val(user_node);
        sea_graphviz_visit_node(fp, temp, map, user);
        // fprintf(fp, "n%p -> n%p [color=\"blue\", dir=back];\n", user, node);
        // fprintf(fp, "n%p -> n%p [color=\"blue\"];\n", user, node);


    }


    fprintf(fp, "n%p ", node);
    if (sea_node_is_cfg(node)) {
        fprintf(fp, " [label=\"%.*s\", shape=box, style=filled, fillcolor=lightgreen];\n", str8_varg(sea_node_label(temp, node)));
    } else {
        fprintf(fp, " [label=\"%.*s\", shape=circle];\n", str8_varg(sea_node_label(temp, node)));
    }

}

void sea_graphviz(const char *filepath, SeaFunctionGraph *fn) {
    Temp scratch = scratch_begin(0, 0);

    FILE *fp = fopen(filepath, "w");
    SeaNodeMap map = sea_map_init(scratch.arena, 101);

    fprintf(fp, "digraph CFG {\n");
    fprintf(fp, "rankdir=bt\n");


    sea_graphviz_visit_node(fp, scratch.arena, &map, fn->stop);

    fprintf(fp, "}\n");

    int error = fclose(fp);

    #if SEA_SHOW_GRAPH
    String8 cmd = str8f(scratch.arena, "dot -Tpng %s -o %s.png && xdg-open %s.png\0", filepath, filepath, filepath);
    system(cmd.str);
    #endif
    scratch_end(scratch);
}
