#ifndef TREE_PARSER
#define TREE_PARSER

#include "ast.h"
#include "tokenizer.h"


typedef struct TreeParser TreeParser;
struct TreeParser {
    Arena *arena;
    Byte *src;
    TreeToken *tokens;
    U32 tokencount;
    U32 curr;
    TreeReturn *tmp; // remove later
};

TreeToken tree_current_token(TreeParser *p) {
    return p->tokens[p->curr];
}

void tree_advance_token(TreeParser *p) {
    p->curr += 1;
}

TreeNode *tree_parse_urinary(TreeParser *p) {
    TreeToken tok = tree_current_token(p);
    switch (tok.kind) {
        case TreeTokenKind_Int_Lit:

        break;
        default:
            // emit error
    }
}

TreeNode *tree_parse_expr(TreeParser *p) {

}

TreeStmt *tree_parse_stmt(TreeParser *p) {
    TreeToken tok = tree_current_token(p);

    switch (tok.kind) {
        case TreeTokenKind_Return: {
            TreeReturn *ret = arena_push(p->arena, TreeReturn);
            tree_advance_token(p);
            p->tmp = ret; // TODO: remove later
            return &ret->stmt;
        }
        default:
            // emit error
    }

    return NULL;
}

#endif
