#ifndef TREE_FRONT_H
#define TREE_FRONT_H

// #include <base/base_inc.h>
#include <front/tokenizer.h>
#include <sea/sea.h>

typedef struct Parser Parser;
struct Parser {
    Arena *arena;
    String8 filename;
    String8 src;
    Token *tokens;
    U32 tok_count;
    U32 curr;
};


int frontend_init(void);
void frontend_deinit();


Parser parser_begin(String8 filename);
Token current_token(Parser *p);
SeaDataType *parse_type(Parser *p);

#endif // TREE_FRONT_H
