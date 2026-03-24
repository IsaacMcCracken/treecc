#ifndef SEA_NODE_H
#define SEA_NODE_H
#include <base/base_inc.h>

#define CTRL_STR str8_lit("@ctrl")

typedef struct SeaModule SeaModule;
typedef struct SeaNode SeaNode;
typedef struct SeaTypeLattice SeaTypeLattice;
typedef struct SeaType SeaType;
typedef struct SeaAllocator SeaAllocator;




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
    SeaType *ret_type;
};


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
    SeaLatticeKind_IntTop,
    SeaLatticeKind_Int,
    SeaLatticeKind_Tuple,
    SeaLatticeKind_Struct,
    SeaLatticeKind_Func,
};


typedef struct SeaTypeInt SeaTypeInt;
struct SeaTypeInt {
    S64 min;
    S64 max;
};

typedef struct SeaTypeArray SeaTypeArray;
struct SeaTypeArray {
    SeaType **items; // must not converge to bottom all same data type
    U64 len;
};

typedef struct SeaTypeStruct SeaTypeStruct;
struct SeaTypeStruct {
    String8 name;
    SeaFieldArray fields;
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
        SeaTypeStruct s;
        SeaFunctionProto func;
        SeaTypeInt i;
        F64 f64;
        F32 f32;
        SeaTypeTuple tup;
    };
};



typedef struct SeaSymbolEntry SeaSymbolEntry;
struct SeaSymbolEntry {
    SeaSymbolEntry *next;
    SeaSymbolEntry *next_hash;
    String8 name;
    SeaType *type;
    U64 pos_in_section;
};

typedef struct SeaUnresolvedEntry SeaUnresolvedEntry;
struct SeaUnresolvedEntry {
    SeaSymbolEntry *next;
    SeaSymbolEntry *next_hash;
    String8 name;
    U64 pos_in_section;
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

typedef struct SeaUnresolvedSymbols SeaUnresolvedSymbols;
struct SeaUnresolvedSymbols {
    Arena *arena;
    SeaSymbolEntry **cells;
    U64 cap;
    U64 count;
    SeaSymbolEntry *first;
    SeaSymbolEntry *last;
};

struct SeaTypeLattice {
    RWMutex lock;
    Arena *arena;
    SeaType **cells;
    U64 cap;
};

typedef U16 SeaNodeKind;
typedef enum {
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
    SeaNodeKind_Dead,
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
    SeaNodeKind_ConstInt,
    SeaNodeKind_Proj,
    SeaNodeKind_Phi,
    SeaNodeKind_Call,
    //*****************//
    // Mem Nodes
    //*****************//
    SeaNodeKind_AllocA,
    SeaNodeKind_Load,

    //*****************//
    // Mem Nodes
    //*****************//

    SeaNodeKind_Copy,
    SeaNodeKind_Move,

    SeaNodeKind_COUNT,
    SeaNodeMachStart = 0x100,
} SeaEnum;


typedef struct SeaUser SeaUser;
struct SeaUser {
    SeaUser *next;
    struct {
      U64 n: 48;
      U64 slot : 16;
    };
};

struct SeaNode {
    SeaNodeKind kind;
    U16 inputcap;
    U16 inputlen;
    U16 idepth;
    U32 gvn;
    U32 nid;
    SeaNode **inputs;
    SeaUser *users;
    union {
        S64 vint;
        void *vptr;
        SeaType *vtype;
    };
    SeaType *type;
};

typedef struct SeaWorkList SeaWorkList;
struct SeaWorkList {
    SeaNode **items;
    U64 itemlen;
    U64 itemcap;
    U64 hashcap;
    U32 *hashset; // gvn as key
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

typedef struct SeaScopeData SeaScopeData;
struct SeaScopeData {
    SeaScopeData *next; // useful for merging
    SeaScopeData *prev; // useful for going forward
    SeaScopeSymbolCell **cells; // buckets for table lookup
    U64 cap; // capacity
    SeaScopeSymbolCell *head; // first symbol (for iteration)
    SeaScopeSymbolCell *tail; // last symbol (for appending)
    U64 symbol_count;
    U16 inputlen;
};

typedef struct SeaScopeList SeaScopeList;
struct SeaScopeList {
    SeaScopeData *head;
    SeaScopeData *tail;
    U64 count;
};


typedef struct SeaScopeManager SeaScopeManager;
struct SeaScopeManager {
    Arena *arena; // allocator of scope data probably in parser
    SeaNode *curr; // current scope
    SeaScopeSymbolCell *cellpool; // free list for cells
    SeaScopeData *scopepool; // free list for scopes
    U64 default_cap; // capacity for new scopes
};

typedef struct SeaBlock SeaBlock;
typedef struct SeaBlockList SeaBlockList;
struct SeaBlockList {
    SeaBlock *head, *tail;
};
struct SeaBlock {
    SeaBlockList children;
    SeaBlock *next, *prev;
    SeaNode *begin;
    SeaNode *end;
    SeaNode **nodes;
    U32 idx;
    U16 nodelen;
    U16 nodecap;
};


typedef struct SeaFunctionGraph SeaFunctionGraph;
struct SeaFunctionGraph {
    SeaModule *m;
    Arena *arena;
    SeaAllocator *alloc;
    SeaFunctionProto proto;
    U64 deadspace;
    SeaNodeMap map;
    SeaNode *start;
    SeaNode *stop;
    U64 node_count;
    U64 nidcap;
    // Optimization Data
    SeaBlock *domtree;
    SeaNode **sched;
    U16 schedcap;
    U16 schedlen;
    void *reg_data;
    SeaWorkList *wl;

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

typedef struct SeaEmitter SeaEmitter;
struct SeaEmitter {
    Arena *arena;
    U8 *code;
    U64 len;
};

struct SeaModule {
    RWMutex lock;
    SeaSymbols symbols;
    SeaFunctionGraphList functions;
    SeaTypeLattice *lat;
    SeaEmitter emit;
};



extern SeaType sea_type_S64;
extern SeaType sea_type_Top;
extern SeaType sea_type_Bot;
extern SeaType sea_type_CtrlLive;
extern SeaType sea_type_CtrlDead;
extern SeaType sea_type_Bool;

// Module
SeaModule sea_create_module(void);
SeaSymbolEntry *sea_lookup_symbol(SeaModule *m, String8 name);
SeaFunctionGraph *sea_add_function(SeaModule *mod, SeaScopeManager *m, SeaFunctionProto proto);
void sea_add_function_symbol(SeaModule *m, SeaFunctionProto proto);
void sea_add_struct_symbol(SeaModule *m, SeaTypeStruct s);
void sea_codegen_module(SeaModule *m);


void sea_node_print_expr_debug(SeaNode *expr);

// Utils
void sea_node_keep(SeaFunctionGraph *fn, SeaNode *node);
void sea_node_unkeep(SeaFunctionGraph *fn, SeaNode *node);
void sea_add_return(SeaFunctionGraph *fn, SeaNode *ret);
B32 sea_node_is_cfg(SeaNode *node);
void sea_node_set_ctrl(SeaFunctionGraph *fn, SeaNode *node, SeaNode *ctrl);
SeaNode *sea_node_get_ctrl(SeaFunctionGraph *fn, SeaNode *node);
void sea_graphviz(const char *filepath, SeaFunctionGraph *fn);
void *sea_alloc(SeaFunctionGraph *fn, U64 size);
#define sea_alloc_item(fn, T) sea_alloc(fn, sizeof(T))
#define sea_alloc_array(fn, T, count) sea_alloc(fn, sizeof(T)*(count))
void sea_free(SeaFunctionGraph *fn, void *ptr, U64 size);
#define sea_free_item(fn, item) sea_free(fn, item, sizeof(*(item)))
#define sea_free_array(fn, arr, count) sea_free(fn, arr, sizeof(*(arr))*(count))

// initalization
SeaNodeMap sea_map_init(Arena *arena, U64 map_cap);
SeaNode *sea_map_lookup(SeaNodeMap *map, SeaNode *node);
void sea_map_insert(SeaNodeMap *map, SeaNode *node);
// Optimizations
SeaNode *sea_peephole(SeaFunctionGraph *fn, SeaNode *node);

// Node Builder Functions
SeaNode *sea_create_stop(SeaFunctionGraph *fn, U16 input_reserve);
SeaNode *sea_create_start(SeaFunctionGraph *fn, SeaScopeManager *m, SeaFunctionProto proto);
SeaNode *sea_create_const_int(SeaFunctionGraph *fn, S64 v);
SeaNode *sea_create_urnary_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *input);
SeaNode *sea_create_bin_op(SeaFunctionGraph *fn, SeaNodeKind kind, SeaNode *lhs, SeaNode *rhs);

SeaNode *sea_create_return(SeaFunctionGraph *fn, SeaNode *prev_ctrl, SeaNode *expr);
SeaNode *sea_create_proj(SeaFunctionGraph *fn, SeaNode *input, U64 v);
SeaNode *sea_create_if(SeaFunctionGraph *fn, SeaNode *ctrl, SeaNode *cond);
SeaNode *sea_create_loop(SeaFunctionGraph *fn, SeaNode *prev_ctrl);
SeaNode *sea_create_dead_ctrl(SeaFunctionGraph *fn);
SeaNode *sea_create_region(SeaFunctionGraph *fn, SeaNode **inputs, U16 ctrl_count);
SeaNode *sea_create_phi2(SeaFunctionGraph *fn, SeaNode *region, SeaNode *a, SeaNode *b);

SeaNode *sea_create_func_call(SeaFunctionGraph *fn, SeaFunctionProto proto, SeaNode **args, U16 arg_count);
SeaNode *sea_create_array_from_items(SeaFunctionGraph *fn, SeaNode *items, U16 len);
// SeaNode *sea_create_array_zero(Seafunc)
SeaNode *sea_create_array_store(SeaFunctionGraph *fn, SeaNode *item, U16 slot);

// Scope Functionality For Building SSA
void sea_begin_scope(SeaScopeManager *m);
void sea_end_scope(SeaFunctionGraph *fn, SeaScopeManager *m);
void sea_scope_free(SeaFunctionGraph *fn, SeaScopeManager *m, SeaNode *scope);
SeaNode *sea_create_scope(SeaScopeManager *m, U16 input_reserve);
void sea_push_scope(SeaScopeManager *m);
void sea_pop_scope(SeaFunctionGraph *fn, SeaScopeManager *m);
void sea_scope_insert_symbol(SeaFunctionGraph *fn, SeaScopeManager *m, String8 name, SeaNode *node);
void sea_scope_update_symbol(SeaFunctionGraph *fn, SeaScopeManager *m, String8 name, SeaNode *node);
SeaNode *sea_scope_lookup_symbol(SeaScopeManager *m, String8 name);
SeaNode *sea_duplicate_scope(SeaFunctionGraph *fn, SeaScopeManager *m, B32 isloop);
SeaNode *sea_merge_scopes(SeaFunctionGraph *fn, SeaScopeManager *m, SeaNode *that_scope);
void sea_scope_end_loop(SeaFunctionGraph *fn, SeaScopeManager *m, SeaNode *head, SeaNode *back, SeaNode *exit);
#endif
