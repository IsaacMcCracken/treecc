#ifndef TREE_TOKENIZER
#define TREE_TOKENIZER

#include "../include/core.h"

typedef U32 TreeTokenKind;
enum {
  TreeTokenKind_Invalid,

  TreeTokenKind_Semi_Colon,

  TreeTokenKind_Plus,
  TreeTokenKind_Minus,
  TreeTokenKind_Star,
  TreeTokenKind_Slash,

  TreeTokenKind_Identifier,
  TreeTokenKind_Int_Lit,

  TreeTokenKind_Return,
  TreeTokenKind_EOF,
};

typedef struct TreeToken TreeToken;
struct TreeToken {
  TreeTokenKind kind;
  U32 start, end;
};



U32 tree_hash_string(String str);
TreeToken *tree_tokenize(Arena *arena, U32 *tokencount, Byte *src, U32 srclen);
void print_tokens(TreeToken *tokens, U32 count, Byte *src);
#endif
