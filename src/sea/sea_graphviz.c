#include "sea_internal.h"


#define SHOW

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
        case SeaNodeKind_NegateI:      return str8_lit("negate");
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
            if (node->inputs[0]->kind == SeaNodeKind_If) {
                if (!node->vint) return str8_lit("false");
                else return str8_lit("true");
            } else {
                SeaType *type = node->inputs[0]->type->tup.elems[node->vint];

                switch (type->kind) {
                    case SeaLatticeKind_Int: return str8_lit("int");
                    case SeaLatticeKind_CtrlLive: return str8_lit("ctrl");
                    case SeaLatticeKind_CtrlDead: return str8_lit("xctrl (x_x)");
                }

                return str8f(temp, "proj(%d)", (int)node->vint);
            }
        }
        case SeaNodeKind_Phi:          return str8_lit("phi");
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
            // fprintf(fp, "n%p -> n%p [dir=back];\n", input, node);
            fprintf(fp, "n%p -> n%p;\n", input, node);
        }
    }


    for EachNode(user_node, SeaUser, node->users) {
        SeaNode *user = sea_user_val(user_node);
        sea_graphviz_visit_node(fp, temp, map, user);
        // fprintf(fp, "n%p -> n%p [color=\"blue\", dir=back];\n", user, node);
        fprintf(fp, "n%p -> n%p [color=\"blue\"];\n", user, node);


    }

    fprintf(fp, "n%p", node);
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


    String8 cmd = str8f(scratch.arena, "dot -Tpng %s -o output.png && xdg-open output.png\0", filepath);
    system(cmd.str);
    scratch_end(scratch);
}
