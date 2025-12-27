#ifndef TREE_NODE_H
#define TREE_NODE_H
#include <core.h>

typedef U16 TreeDataType;
enum {
  TreeDataType_Top,
  TreeDataType_Bottom,
  TreeDataType_I64,
  TreeDataType_Control,
  TreeDataType_DeadControl,
};



typedef U16 TreeNodeKind;
enum {
    TreeNodeKind_Invalid,

    TreeNodeKind_SymbolTable,

    //*****************//
    // Control Flow
    //*****************//

    TreeNodeKind_Start,
    TreeNodeKind_Stop,
    TreeNodeKind_Return,
    TreeNodeKind_If,
    TreeNodeKind_Region,
    TreeNodeKind_Loop,

    //*****************//
    // Data Operations
    //*****************//

    // Integer Arithmetic
    TreeNodeKind_AddI,
    TreeNodeKind_SubI,
    TreeNodeKind_NegI,
    TreeNodeKind_MulI,
    TreeNodeKind_DivI,
    TreeNodeKind_NegateI,

    // Logic
    TreeNodeKind_Not,
    TreeNodeKind_EqualI,
    TreeNodeKind_NotEqualI,
    TreeNodeKind_GreaterThanI,
    TreeNodeKind_GreaterEqualI,
    TreeNodeKind_LesserThanI,
    TreeNodeKind_LesserEqualI,

    // Data
    TreeNodeKind_ConstInt,
    TreeNodeKind_Proj,
    TreeNodeKind_Phi,
};


typedef struct TreeNode TreeNode;

typedef struct TreeUser TreeUser;
struct TreeUser {
  TreeNode *n;
  U16 slot;
};

struct TreeNode {
    TreeNodeKind kind;
    TreeDataType type;
    U16 inputcap, usercap;
    U16 inputlen, userlen;
    TreeNode **inputs;
    TreeUser *users;
    union {
        S64 vint;
        void *vptr;
    };
};

typedef struct TreeNodeMapCell TreeNodeMapCell;
struct TreeNodeMapCell {
    TreeNode *node;
    TreeNodeMapCell *next;
};

typedef struct TreeNodeMap TreeNodeMap;
struct TreeNodeMap {
    TreeNodeMapCell **cells;
    U64 cap;
    TreeNodeMapCell *freelist;
    Arena *arena;
};

typedef struct TreeScopeSymbolCell TreeScopeSymbolCell;
struct TreeScopeSymbolCell {
    TreeScopeSymbolCell *hash_next; // next in hash buck
    TreeScopeSymbolCell *next; // next symbol inserted (for iteration);
    String name;
    U16 slot;
};

typedef struct TreeScopeNode TreeScopeNode;
struct TreeScopeNode {
    TreeNode *prev; // previous scope / parent
    TreeScopeSymbolCell **cells; // buckets for table lookup
    U64 capacity; // capacity
    TreeScopeSymbolCell *head; // first symbol (for iteration);
    TreeScopeSymbolCell *tail; // last symbol (for appending);
    U64 symbol_count;
};


typedef struct TreeScopeManager TreeScopeManager;
struct TreeScopeManager {
    TreeScopeSymbolCell *cellpool; // free list for cells
    TreeScopeNode *scopepool; // free list for scopes
    U64 default_cap; // capacity for new scopes
};


typedef struct TreeFunctionGraph TreeFunctionGraph;
struct TreeFunctionGraph {
    Arena *arena;
    Arena *tmp;
    U64 deadspace;
    TreeScopeManager mscope;
    TreeNode *scope;
    TreeNodeMap map;
    TreeNode *start;
    TreeNode *stop;
};




// temporary

void tree_node_print_expr_debug(TreeNode *expr);

// initalization
TreeNodeMap tree_map_init(Arena *arena, U64 map_cap);
U32 tree_hash_dbj2(Byte *data, U64 len);

//------------------------------------------//
// Builder Functions
//------------------------------------------//

// scope functions
TreeScopeManager tree_scope_manager_init(U64 default_cap);
TreeNode *tree_push_new_scope(TreeFunctionGraph *fn, TreeNode *ctrl, TreeNode *prev);
TreeNode *tree_duplicate_scope(TreeFunctionGraph *fn, TreeNode *original);

// expression functions
TreeNode *tree_create_const_int(TreeFunctionGraph *fn, S64 v);
TreeNode *tree_create_urnary_expr(TreeFunctionGraph *fn, TreeNodeKind kind, TreeNode *input);
TreeNode *tree_create_binary_expr(TreeFunctionGraph *fn, TreeNodeKind kind, TreeNode *lhs, TreeNode *rhs);

TreeNode *tree_create_proj(TreeFunctionGraph *fn, TreeNode *input, U16 v);

// control flow
TreeNode *tree_create_return(TreeFunctionGraph *fn, TreeNode *prev_ctrl, TreeNode *expr);

TreeNode *tree_create_if(TreeFunctionGraph *fn, TreeNode *prev_ctrl);
TreeNode *tree_create_region_for_if(TreeFunctionGraph *fn, TreeNode *t, TreeNode *f, U16 output_reserves);
TreeNode *tree_create_phi2(TreeFunctionGraph *fn, TreeNode *region, TreeNode *a, TreeNode *b);

#endif
