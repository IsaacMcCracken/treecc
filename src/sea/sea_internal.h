#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H

#include "sea.h"

extern SeaType sea_type_IfBoth;
extern SeaType sea_type_IfNeth;
extern SeaType sea_type_IfTrue;
extern SeaType sea_type_IfFalse;

SeaNode *sea_create_scope_with_cap(SeaFunctionGraph *fn, U16 input_reserve);
SeaDataKind sea_get_data_kind(SeaNode *node);
SeaNode *sea_create_dead_ctrl(SeaFunctionGraph *fn, SeaNode *input);
void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot);
U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input);
void sea_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new);


// Type Stuff
// TODO(ISAAC) move lattice to module??
void sea_lattice_init(SeaFunctionGraph *fn);
void sea_lattice_insert(SeaFunctionGraph *fn, SeaType *t);
void sea_lattice_raw_insert(SeaFunctionGraph *fn, SeaType *t);

// type creators
SeaType *sea_type_const_int(SeaFunctionGraph *fn, S64 v);
SeaType *sea_type_tuple(SeaFunctionGraph *fn, SeaType **elems, U64 count);

// type computers
SeaType *sea_compute_type(SeaFunctionGraph *fn, SeaNode *n);
SeaType *compute_int_bin_op(SeaFunctionGraph *fn, SeaNode *n);
SeaType *sea_compute_if(SeaFunctionGraph *fn, SeaNode *ifnode);
SeaType *compute_int_urnary_op(SeaFunctionGraph *fn, SeaNode *n);
SeaType *sea_type_meet(SeaFunctionGraph *fn, SeaType *a, SeaType *b);

// user type checks -- TODO (Isaac) move to sea.h??
B32 sea_type_is_const_int(SeaType *t);
S64 sea_type_const_int_val(SeaType *t);
B32 sea_type_is_const_int(SeaType *t);



#endif // SEA_INTERNAL_H
