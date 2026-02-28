#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H

#include "sea.h"

typedef struct FreeNode FreeNode;
struct FreeNode {
    FreeNode *next;
};

struct SeaAllocator {
    Arena *arena;
    FreeNode *small_buckets[8]; // 0 - 64 bytes in increments of 8
    FreeNode *huge_buckets[6]; // 128 to 4096 bytes in increments of powers of 2
};



extern SeaType sea_type_IfBoth;
extern SeaType sea_type_IfNeth;
extern SeaType sea_type_IfTrue;
extern SeaType sea_type_IfFalse;

// Node stuff
void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot);
U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input);
void sea_node_remove_input(SeaFunctionGraph *fn, SeaNode *node, U16 slot);
void sea_node_remove_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot);
void sea_node_kill(SeaFunctionGraph *fn, SeaNode *node);
void sea_node_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new);
SeaNode *sea_user_val(SeaUser *user);
U16 sea_user_slot(SeaUser *user);

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
