#include "sea_internal.h"



void sea_codegen_fn_graph(SeaModule *m, SeaFunctionGraph *fn) {
    // TODO iterative peepholes
    sea_instruction_selection(fn);
    sea_global_code_motion(fn);
    sea_ssa_deconstruction(fn);
    sea_local_schedule(fn);
    sea_reg_alloc(fn);
    sea_encode(m, fn);

}


void sea_codegen_module(SeaModule *m) {
    for EachNode(fn_node, SeaFunctionGraphNode, m->functions.first) {
        sea_codegen_fn_graph(m, &fn_node->fn);
    }
}
