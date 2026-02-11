#ifndef SEA_NODE_H
#define SEA_NODE_H
#include <base/base_inc.h>

#define CTRL_STR str8_lit("@ctrl")

typedef struct SeaModule SeaModule;
typedef struct SeaNode SeaNode;
typedef struct SeaTypeLattice SeaTypeLattice;
typedef struct SeaType SeaType;


typedef U8 SeaDataKind;
enum {
  SeaDataKind_Void,
  SeaDataKind_I64,
  SeaDataKind_Ctrl,
  SeaDataKind_Memory,
  SeaDataKind_COUNT,
};

typedef U8 SeaLatticeKind;
enum {
    SeaLatticeKind_Top,
    SeaLatticeKind_Bot,
    SeaLatticeKind_CtrlLive,
    SeaLatticeKind_CtrlDead,
    SeaLatticeKind_SIMPLE,
    SeaLatticeKind_Int,
    SeaLatticeKind_Tuple,
};


typedef struct SeaTypeInt SeaTypeInt;
struct SeaTypeInt {
    S64 min;
    S64 max;
};

typedef struct SeaTypeTuple SeaTypeTuple;
struct SeaTypeTuple {
    SeaType **elems;
    U64 count;
};

typedef struct SeaType SeaType;
struct SeaType {
    SeaType *hash_next;
    SeaLatticeKind kind;
    union {
        SeaTypeInt i;
        F64 f64;
        F32 f32;
        SeaTypeTuple tup;
    };
};

struct SeaTypeLattice {
    SeaType **cells;
    U64 cap;
};



typedef struct SeaField SeaField;
struct SeaField {
    String8 name;
    SeaType *type;
};

typedef struct SeaFieldArray SeaFieldArray;
struct SeaFieldArray {
    SeaField *fields;
    U64 count;
};

typedef struct SeaFunctionProto SeaFunctionProto;
struct SeaFunctionProto {
    String8 name;
    SeaFieldArray args;
};

typedef U8 SeaSymbolKind;
enum {
    SeaSymbolKind_Invalid,
    SeaSymbolKind_Function,
    SeaSymbolKind_Global,
};

typedef struct SeaSymbolEntry SeaSymbolEntry;
struct SeaSymbolEntry {
    SeaSymbolEntry *next;
    SeaSymbolEntry *next_hash;
    String8 name;
    SeaSymbolKind kind;
    union {
        SeaFunctionProto fn_proto;
    };
};

typedef struct SeaSymbols SeaSymbols;
struct SeaSymbols {
    Arena *arena;
    SeaSymbolEntry **cells;
    U64 cap;
    U64 count;
    SeaSymbolEntry *first;
    SeaSymbolEntry *last;
};

typedef U16 SeaNodeKind;
enum {
    SeaNodeKind_Invalid,

    //*****************//
    // Misc
    //*****************//
    SeaNodeKind_Scope,

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
    SeaNodeKind_ModI,
    SeaNodeKind_NegateI,

    // Logic
    SeaNodeKind_Not,
    SeaNodeKind_And,
    SeaNodeKind_Or,
    SeaNodeKind_EqualI,
    SeaNodeKind_NotEqualI,
    SeaNodeKind_GreaterThanI,
    SeaNodeKind_GreaterEqualI,
    SeaNodeKind_LesserThanI,
    SeaNodeKind_LesserEqualI,

    // Bitwise Operations
    SeaNodeKind_BitNotI,
    SeaNodeKind_BitAndI,
    SeaNodeKind_BitOrI,
    SeaNodeKind_BitXorI,


    //*****************//
    // Data Nodes
    //*****************//
    SeaNodeKind_Const,
    SeaNodeKind_Proj,
    SeaNodeKind_Phi,
    SeaNodeKind_COUNT,
};


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
    SeaType *type;
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
    U64 cap; // capacity
    SeaScopeSymbolCell *head; // first symbol (for iteration)
    SeaScopeSymbolCell *tail; // last symbol (for appending)
    U64 symbol_count;
    U64 depth;
};


typedef struct SeaScopeManager SeaScopeManager;
struct SeaScopeManager {
    SeaScopeSymbolCell *cellpool; // free list for cells
    SeaNode *scopepool; // free list for scopes
    U64 default_cap; // capacity for new scopes
};


typedef struct SeaFunctionGraph SeaFunctionGraph;
struct SeaFunctionGraph {
    SeaModule *m;
    Arena *arena;
    Arena *tmp;
    SeaFunctionProto proto;
    U64 deadspace;
    SeaScopeManager mscope;
    SeaNode *scope;
    SeaNodeMap map;
    SeaTypeLattice *lat;
    SeaNode *start;
    SeaNode *stop;
    SeaNode *curr; // only used in graph creation
};

typedef struct SeaFunctionGraphNode SeaFunctionGraphNode;
struct SeaFunctionGraphNode {
    SeaFunctionGraphNode *next;
    SeaFunctionGraph fn;
};


typedef struct SeaFunctionGraphList SeaFunctionGraphList;
struct SeaFunctionGraphList {
    SeaFunctionGraphNode *first;
    SeaFunctionGraphNode *last;
    U64 count;
};

struct SeaModule {
    RWMutex lock;
    SeaSymbols symbols;
    SeaFunctionGraphList functions;
};


extern SeaType sea_type_S64;
extern SeaType sea_type_Top;
extern SeaType sea_type_Bot;
extern SeaType sea_type_CtrlLive;
extern SeaType sea_type_CtrlDead;

// Module
SeaModule sea_create_module(void);
SeaFunctionGraph *sea_add_function(SeaModule *m, SeaFunctionProto proto);
void sea_add_function_symbol(SeaModule *m, SeaFunctionProto proto);


void sea_node_print_expr_debug(SeaNode *expr);

// initalization
SeaNodeMap sea_map_init(Arena *arena, U64 map_cap);
// U32 sea_hash_dbj2(Byte *data, U64 len);

// Builder Functions

SeaNode *sea_create_stop(SeaFunctionGraph *fn, U16 input_reserve);
SeaNode *sea_create_start(SeaFunctionGraph *fn, SeaFunctionProto proto);

SeaNode *sea_create_const_int(SeaFunctionGraph *fn, S64 v);
SeaNode *sea_create_urnary_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input);
SeaNode *sea_create_bin_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *lhs, SeaNode *rhs);

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr);
SeaNode *sea_create_proj(SeaFunctionGraph *fn, SeaNode *input, U64 v);
SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *ctrl, SeaNode *cond);


SeaNode *sea_create_region(SeaFunctionGraph *fn, SeaNode **ctrl_ins, U16 ctrl_count, U16 output_reserves);
SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b);


void sea_push_new_scope(SeaFunctionGraph *fn);
void sea_pop_scope(SeaFunctionGraph *fn, SeaNode *scope);
void sea_pop_this_scope(SeaFunctionGraph *fn);
void sea_free_all_scopes(SeaFunctionGraph *fn, SeaNode *scope);
SeaNode *sea_duplicate_scope(SeaFunctionGraph *fn, SeaNode *original);
void sea_scope_insert_symbol(SeaFunctionGraph *fn, SeaNode *scope, String8 name, SeaNode *node);
void sea_update_local_symbol(SeaFunctionGraph *fn, String8 name, SeaNode *node);
void sea_insert_local_symbol(SeaFunctionGraph *fn, String8 name, SeaNode *node);
SeaNode *sea_lookup_local_symbol(SeaFunctionGraph *fn, String8 name);
SeaNode *sea_scope_lookup_symbol(SeaFunctionGraph *fn, SeaNode *scope, String8 name);
SeaNode *sea_merge_scopes(SeaFunctionGraph *fn, SeaNode *this_scope, SeaNode *that_scope, SeaNode **out_scope);
#endif
