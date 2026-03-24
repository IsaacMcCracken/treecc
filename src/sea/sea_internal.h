#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H

#include "sea.h"

#define SEA_REGMASK_WORDS 4
#define SEA_REGMASK_BITS  (SEA_REGMASK_WORDS * 64)

typedef struct FreeNode FreeNode;
struct FreeNode {
    FreeNode *next;
};

struct SeaAllocator {
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

typedef struct RegMask RegMask;
struct RegMask {
  U64 m[4];
};

typedef struct CallConv CallConv;
struct CallConv {
    U8 args[8];
    U8 arglen;
};

typedef S64 (*SeaEncodeNode)(SeaEmitter *, SeaFunctionGraph *, SeaNode *);
typedef SeaNode *(*SeaMachInstrSelectFn)(SeaFunctionGraph *, SeaNode *);
typedef SeaNode *(*SeaMachJumpFn)(SeaFunctionGraph *, S64 pos);
typedef SeaNode *(*SeaMachSplitFn)(SeaFunctionGraph *);
typedef RegMask (*SeaMachRegOutFn)(SeaNode *);
typedef RegMask (*SeaMachRegInFn)(SeaNode *, U16);

typedef struct SeaMach SeaMach;
    struct SeaMach {
    String8 name;
    SeaEncodeNode encode;
    SeaMachInstrSelectFn select;
    SeaMachJumpFn jump;
    SeaMachSplitFn split;
    SeaMachRegOutFn rmask_out;
    SeaMachRegInFn rmask_in;
    CallConv conv;
};

extern SeaType sea_type_IfBoth;
extern SeaType sea_type_IfNeth;
extern SeaType sea_type_IfTrue;
extern SeaType sea_type_IfFalse;

extern SeaMach mach;


// Node stuff
SeaNode *sea_node_alloc(SeaFunctionGraph *fn, SeaNodeKind kind, U16 inputcap, U16 inputlen);
void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot);
void sea_node_append_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot);
U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input);
void sea_node_remove_input(SeaFunctionGraph *fn, SeaNode *node, U16 slot);
void sea_node_remove_user(SeaFunctionGraph *fn, SeaNode *node, SeaNode *user, U16 slot);
void sea_node_kill(SeaFunctionGraph *fn, SeaNode *node);
void sea_node_subsume(SeaFunctionGraph *fn, SeaNode *old, SeaNode *new);
SeaNode *sea_user_val(SeaUser *user);
U16 sea_user_slot(SeaUser *user);
void sea_node_set_input_raw(SeaNode *node, SeaNode *input, U16 slot);
void sea_node_remove_all_users_raw(SeaNode *node);
U16 sea_node_count_users(SeaNode *n);

// Type Stuff
void sea_lattice_init(SeaModule *m);
void sea_lattice_insert(SeaFunctionGraph *fn, SeaType *t);

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

// Reg Mask Stuff
static inline RegMask rmask_u64(U64 mask);
static inline RegMask rmask_empty(void);
static inline RegMask rmask_full(void);
static inline B32     rmask_get(RegMask rm, U8 reg);
static inline RegMask rmask_set(RegMask rm, U8 reg);
static inline RegMask rmask_unset(RegMask rm, U8 reg);
static inline RegMask rmask_and(RegMask a, RegMask b);
static inline RegMask rmask_or(RegMask a, RegMask b);
static inline RegMask rmask_not(RegMask a);
static inline B32     rmask_empty_p(RegMask rm);
static inline B32     rmask_eq(RegMask a, RegMask b);
static inline S32     rmask_get_first_empty(RegMask rm);


// Codegen Phases
void sea_instruction_selection(SeaFunctionGraph *fn);
void sea_global_code_motion(SeaFunctionGraph *fn);
void sea_ssa_deconstruction(SeaFunctionGraph *fn);
void sea_list_schedule(SeaFunctionGraph *fn);
void sea_reg_alloc(SeaFunctionGraph *fn);
void sea_encode(SeaModule *m, SeaFunctionGraph *fn);

// scheduling info
B32 mach_node_is_cfg(SeaNode *n);
B32 node_is_blockhead(SeaNode *cfg);
SeaNode *cfg_zero(SeaNode *n);
B32 sea_node_is_mach(SeaNode *node);
B32 sea_node_is_bool(SeaNode *node);
B32 is_forward_edge(SeaNode *u, SeaNode *d);

// codegen
SeaEmitter emitter_init(Arena *arena);
void emitter_push_bytes(SeaEmitter *e, U8 *bytes, U64 len);
void emitter_push_byte(SeaEmitter *e, U8 b);
void emitter_push_s32(SeaEmitter *e, S32 x);
void emitter_push_s64(SeaEmitter *e, S64 x);
B32 sea_set_symbol_pos(SeaModule *m, String8 name, U64 pos);
S32 sea_get_reg_colour(SeaFunctionGraph *fn, SeaNode *n);
static B32 is_2addr(SeaNode *n);
// dumb print stuff
String8 sea_node_instr_label(Arena *temp, SeaNode *node);


#endif // SEA_INTERNAL_H
