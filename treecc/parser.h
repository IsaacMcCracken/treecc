#ifndef TREE_PARSER
#define TREE_PARSER

#include "tokenizer.h"
#include "c_types.h"
#include "node.h"
#include "scope_symbol_table.h"

typedef U16 TreeDeclKind;
enum {
    TreeDeclKind_Invalid,
    TreeDeclKind_Struct,
    TreeDeclKind_FnProto,
    TreeDeclKind_Fn,
    TreeDeclKind_Global,
};

typedef struct TreeDecl TreeDecl;
struct TreeDecl {
    TreeDeclKind kind;
    U32 tokindex;
    TreeDecl *prev, *next;
};

typedef struct TreeFnDecl TreeFnDecl;
struct TreeFnDecl {
    TreeDeclKind kind;
    U32 tokindex;
    TreeDecl *prev, *next;
    // Internal

    TreeFunction *type;
    TreeFunctionGraph g; // what the frick is this name man
};


typedef struct TreeSymbolCell TreeSymbolCell;
struct TreeSymbolCell {
  TreeSymbolCell *next;
  String name;
  TreeDecl *decl;
};

typedef struct TreeSymbolTable TreeSymbolTable;
struct TreeSymbolTable {
    TreeSymbolCell **cells;
    U64 capacity;
    Arena *arena;
};

typedef struct TreeParser TreeParser;
struct TreeParser {
    Arena *arena; // prolly remove this cuz I think the function is gonnna alloate the nodes
    Byte *src;
    TreeToken *tokens;
    TreeScopeManager scopes;
    TreeScopeTable *current_scope;
    TreeSymbolTable symbols;
    TreeTypeMap types;
    U32 tokencount;
    U32 linenum;
    U32 linetok;
    U32 curr;
    TreeFunctionGraph fn;
    TreeNode *ret;
};

TreeNode *tree_parse_expr(TreeParser *p);
TreeNode *tree_parse_stmt(TreeParser *p, TreeNode *prev_ctrl);
void tree_parse_scope(TreeParser *p, TreeNode *prev_ctrl);
TreeDecl *tree_parse_decl(TreeParser *p);

#endif
