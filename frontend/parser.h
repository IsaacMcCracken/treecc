#ifndef TREE_PARSER
#define TREE_PARSER

#include "tokenizer.h"
#include <soup.h>

typedef struct TreeParser TreeParser;
struct TreeParser {
    Arena *arena;
    Byte *src;
    TreeToken *tokens;
    U32 tokencount;
    U32 curr;
    SoupFunction fn;
    SoupNode *ret;
};

SoupNode *tree_parse_stmt(TreeParser *p);

#endif
