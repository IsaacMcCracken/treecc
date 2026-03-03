#include "sea_internal.h"



void sea_codegen_fn_graph(SeaFunctionGraph *fn) {
    // TODO iterative peepholes
    sea_global_code_motion(fn);
}


void sea_codegen_module(SeaModule *m) {
    for EachNode(fn_node, SeaFunctionGraphNode, m->functions.first) {
        sea_codegen_fn_graph(&fn_node->fn);
    }
}
