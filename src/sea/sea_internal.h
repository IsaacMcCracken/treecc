#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H

#include "sea.h"

typedef struct FreeNode FreeNode;
struct FreeNode {
    FreeNode *next;
};

struct SeaAllocator {
    Arena *arena;
    FreeNode *small_buckets[8]; // 8 - 64 bytes in increments of 8
    FreeNode *huge_buckets[6]; // 128 to 4096 bytes in increments of powers of 2
};

// I did this cuz its goofy aahhh
typedef struct SeaNodeNode SeaNodeNode;
struct SeaNodeNode {
    SeaNodeNode *next;
    SeaNode *node;
};

typedef struct {
    SeaNodeNode *head;
    SeaNodeNode *tail;
    U64 count;
} SeaNodeList;





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
void sea_lattice_init(SeaModule *m);
void sea_lattice_insert(SeaFunctionGraph *fn, SeaType *t);
// void sea_lattice_raw_insert(SeaFunctionGraph *fn, SeaType *t);

// type creators
SeaType *sea_type_const_int(SeaFunctionGraph *fn, S64 v);
SeaType *sea_type_tuple(SeaFunctionGraph *fn, SeaType **elems, U64 count);
SeaType *sea_type_make_func(SeaModule *m, SeaFunctionProto func);
SeaType *sea_type_make_struct(SeaModule *m, SeaTypeStruct s);

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

U16 sea_node_idepth(SeaNode *n);
SeaNode *sea_node_idom(SeaNode *node);

void sea_node_list_push_tail(SeaNodeList *l, SeaNodeNode *n);
void sea_node_list_push_head(SeaNodeList *l, SeaNodeNode *n);



// Codegen Phases
void sea_global_code_motion(SeaFunctionGraph *fn);
void sea_list_schedule(SeaFunctionGraph *fn);
// scheduling info
B32 node_is_blockhead(SeaNode *cfg);
SeaNode *cfg_zero(SeaNode *n);

// dumb print stuff
String8 sea_node_instr_label(Arena *temp, SeaNode *node);


#endif // SEA_INTERNAL_H
