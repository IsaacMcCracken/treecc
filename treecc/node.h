#ifndef TREE_NODE_H
#define TREE_NODE_H
#include <core.h>

typedef U16 TreeDataType;
enum {
  TreeNodeKind_Top,
  TreeNodeKind_Bottom,
  TreeDataKind_I64,
  TreeDataKind_Control
  TreeDataKind_DeadControl
};



typedef U16 TreeNodeKind;
enum {
    TreeNodeKind_Invalid,

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


typedef struct TreeFunctionGraph TreeFunctionGraph;
struct TreeFunctionGraph {
    Arena *arena;
    U64 deadspace;
    TreeNodeMap map;
    TreeNode *start;
    TreeNode *stop;
};

// temporary

void tree_node_print_expr_debug(TreeNode *expr);

// initalization
TreeNodeMap tree_map_init(Arena *arena, U64 map_cap);
U32 tree_hash_dbj2(Byte *data, U64 len);

// Builder Functions
TreeNode *tree_create_const_int(TreeFunctionGraph *fn, S64 v);
TreeNode *tree_create_urnary_expr(TreeFunctionGraph *fn, TreeNodeKind kind, TreeNode *input);
TreeNode *tree_create_binary_expr(TreeFunctionGraph *fn, TreeDataKind kind, TreeNode *lhs, TreeNode *rhs);

TreeNode *tree_create_return(TreeFunctionGraph *fn, TreeNode *prev_ctrl, TreeNode *expr);
TreeNode *tree_create_proj(TreeFunctionGraph *fn, TreeNode *input, U16 v);

TreeNode *tree_create_if(TreeFunctionGraph *fn, TreeNode *prev_ctrl);
TreeNode *tree_create_region_for_if(TreeFunctionGraph *fn, TreeNode *t, TreeNode *f, U16 output_reserves);
TreeNode *tree_create_phi2(TreeFunctionGraph *fn, TreeNode *region, TreeNode *a, TreeNode *b);

#endif
