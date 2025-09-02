#define CORE_IMPLEMENTATION
#include "../include/core.h"

typedef U32 TreeNodeKind;
enum {
    TreeNodeKind_Invalid,
    // Expressions
    TreeNodeKind_Add,
    TreeNodeKind_Sub,
    TreeNodeKind_Mul,
    TreeNodeKind_Div,
    // Statements
    TreeNodeKind_Return,
    TreeNodeKind_COUNT,
};

typedef struct TreeNode TreeNode;
struct TreeNode {
  TreeNodeKind kind;
};

typedef struct TreeStmt TreeStmt;
struct TreeStmt {
    TreeNodeKind kind;
    TreeNode *prev, *next;
};

typedef struct TreeBinaryExpr TreeBinaryExpr;
struct TreeBinaryExpr {
    TreeNode node;
    TreeNode *lhs;
    TreeNode *rhs;
};

typedef struct TreeReturn TreeReturn;
struct TreeReturn {
    TreeStmt stmt;
    TreeNode *expr;
};
