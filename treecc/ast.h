#define CORE_IMPLEMENTATION
#include "../include/core.h"

typedef U32 TreeTokenIndex;

typedef U32 TreeNodeKind;
enum {
    TreeNodeKind_Invalid,
    // Expressions
    TreeNodeKind_Add,
    TreeNodeKind_Sub,
    TreeNodeKind_Mul,
    TreeNodeKind_Div,
    // Statements
    TreeNodeKind_Int_Const,
    TreeNodeKind_Return,
    TreeNodeKind_COUNT,
};

#define TREE_NODE_SUBTYPE TreeNodeKind kind; TreeTokenIndex index
typedef struct TreeNode TreeNode;
struct TreeNode {
    TREE_NODE_SUBTYPE;
};


#define TREE_STMT_SUBTYPE TREE_NODE_SUBTYPE; TreeNode *prev, *next
typedef struct TreeStmt TreeStmt;
struct TreeStmt {
    TREE_STMT_SUBTYPE;
};

typedef struct TreeBinaryExpr TreeBinaryExpr;
struct TreeBinaryExpr {
    TREE_NODE_SUBTYPE;
    TreeNode *lhs;
    TreeNode *rhs;
};

typedef struct TreeReturn TreeReturn;
struct TreeReturn {
    TREE_STMT_SUBTYPE;
    TreeNode *expr;
};
