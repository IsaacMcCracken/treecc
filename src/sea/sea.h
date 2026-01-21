#ifndef TREE_NODE_H
#define TREE_NODE_H
#include <base/base_inc.h>

typedef U8 SeaDataKind;
enum {
  SeaDataType_Top,
  SeaDataType_Bottom,
  SeaDataType_I32,
  SeaDataType_I64,
  SeaDataType_Control,
  SeaDataType_Mem,
  SeaDataType_DeadControl,
};

typedef struct SeaDataType SeaDataType;
struct SeaDataType {
    SeaDataKind kind;
};

typedef struct SeaField SeaField;
struct SeaField {
    String8 name;
    SeaDataType *type;
};

typedef struct SeaFunctionProto SeaFunctionProto;
struct SeaFunctionProto {
    String8 name;
    SeaField *args;
    U64 arg_count;
};


typedef U16 SeaNodeKind;
enum {
    SeaNodeKind_Invalid,

    SeaNodeKind_SymbolTable,

    //*****************//
    // Control Flow
    //*****************//

    SeaNodeKind_Start,
    SeaNodeKind_Stop,
    SeaNodeKind_Return,
    SeaNodeKind_If,
    SeaNodeKind_Region,
    SeaNodeKind_Loop,

    //*****************//
    // Data Operations
    //*****************//

    // Integer Arithmetic
    SeaNodeKind_AddI,
    SeaNodeKind_SubI,
    SeaNodeKind_NegI,
    SeaNodeKind_MulI,
    SeaNodeKind_DivI,
    SeaNodeKind_NegateI,

    // Logic
    SeaNodeKind_Not,
    SeaNodeKind_EqualI,
    SeaNodeKind_NotEqualI,
    SeaNodeKind_GreaterThanI,
    SeaNodeKind_GreaterEqualI,
    SeaNodeKind_LesserThanI,
    SeaNodeKind_LesserEqualI,

    // Data
    SeaNodeKind_ConstInt,
    SeaNodeKind_Proj,
    SeaNodeKind_Phi,
};


typedef struct SeaNode SeaNode;

typedef struct SeaUser SeaUser;
struct SeaUser {
  SeaNode *n;
  U16 slot;
};

struct SeaNode {
    SeaNodeKind kind;
    U16 inputcap, usercap;
    U16 inputlen, userlen;
    SeaNode **inputs;
    SeaUser *users;
    union {
        S64 vint;
        void *vptr;
    };
    SeaDataType *type;
};

typedef struct SeaNodeMapCell SeaNodeMapCell;
struct SeaNodeMapCell {
    SeaNode *node;
    SeaNodeMapCell *next;
};

typedef struct SeaNodeMap SeaNodeMap;
struct SeaNodeMap {
    SeaNodeMapCell **cells;
    U64 cap;
    SeaNodeMapCell *freelist;
    Arena *arena;
};

typedef struct SeaScopeSymbolCell SeaScopeSymbolCell;
struct SeaScopeSymbolCell {
    SeaScopeSymbolCell *hash_next; // next in hash buck
    SeaScopeSymbolCell *next; // next symbol inserted (for iteration)
    String8 name;
    U16 slot;
};

typedef struct SeaScopeNode SeaScopeNode;
struct SeaScopeNode {
    SeaNode *prev; // previous scope / parent
    SeaScopeSymbolCell **cells; // buckets for table lookup
    U64 capacity; // capacity
    SeaScopeSymbolCell *head; // first symbol (for iteration)
    SeaScopeSymbolCell *tail; // last symbol (for appending)
    U64 symbol_count;
};


typedef struct SeaScopeManager SeaScopeManager;
struct SeaScopeManager {
    SeaScopeSymbolCell *cellpool; // free list for cells
    SeaScopeNode *scopepool; // free list for scopes
    U64 default_cap; // capacity for new scopes
};


typedef struct SeaFunctionGraph SeaFunctionGraph;
struct SeaFunctionGraph {
    Arena *arena;
    Arena *tmp;
    U64 deadspace;
    SeaScopeManager mscope;
    SeaNode *scope;
    SeaNodeMap map;
    SeaNode *start;
    SeaNode *stop;
};




// temporary

void sea_node_print_expr_debug(SeaNode *expr);

// initalization
SeaNodeMap sea_map_init(Arena *arena, U64 map_cap);
// U32 sea_hash_dbj2(Byte *data, U64 len);

// Builder Functions
SeaNode *sea_create_const_int(SeaFunctionGraph *fn, S64 v);
SeaNode *sea_create_urnary_expr(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input);
SeaNode *sea_create_binary_expr(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *lhs, SeaNode *rhs);

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr);
SeaNode *sea_create_proj(SeaFunctionGraph *fn, SeaNode *input, U16 v);

SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *prev_ctrl);
SeaNode *sea_create_region_for_if(SeaFunctionGraph *fn, SeaNode *t, SeaNode *f, U16 output_reserves);
SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b);

#endif
