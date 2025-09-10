#ifndef TREE_PARSER
#define TREE_PARSER

#include <soup.h>
#include "tokenizer.h"
#include "types.h"

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
    SoupFunction soup; // what the frick is this name man
};


typedef struct TreeSymbolCell TreeSymbolCell;
struct TreeSymbolCell {
  TreeSymbolCell *next;
  String name;
  TreeDecl *decl;
};

typedef struct TreeSymbolTable TreeSymbolTable;
struct TreeSymbolTable {
    U64 capacity;

};

typedef struct TreeParser TreeParser;
struct TreeParser {
    Arena *arena;
    Byte *src;
    TreeToken *tokens;
    TreeTypeMap types;
    U32 tokencount;
    U32 linenum;
    U32 linetok;
    U32 curr;
    SoupFunction fn;
    SoupNode *ret;
};

SoupNode *tree_parse_stmt(TreeParser *p);
TreeDecl *tree_parse_decl(TreeParser *p);

#endif
