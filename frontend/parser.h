#ifndef TREE_PARSER
#define TREE_PARSER

#include <soup.h>
#include "tokenizer.h"
#include "types.h"

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
