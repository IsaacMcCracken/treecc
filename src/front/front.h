#ifndef TREE_FRONT_H
#define TREE_FRONT_H

#include <base/base_arena.h>
#include <sea/sea.h>
#include <front/tokenizer.h>

typedef struct Parser Parser;
struct Parser {
    Arena *arena;
    String filename;
    String src;
    Token *tokens;
    U32 tok_count;
    U32 curr;
};


int frontend_init(void);
void frontend_deinit()


Parser parser_begin(String filename);
Token current_token(Parser *p);
SeaDataType *parse_type(Parser *p);

#endif // TREE_FRONT_H
