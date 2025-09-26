#ifndef TREE_TOKENIZER
#define TREE_TOKENIZER

#include <core.h>

typedef U32 TreeTokenKind;
enum {
  TreeTokenKind_Invalid,

  TreeTokenKind_Int,

  TreeTokenKind_Equals,
  TreeTokenKind_SemiColon,
  TreeTokenKind_Comma,

  TreeTokenKind_LParen,
  TreeTokenKind_RParen,
  TreeTokenKind_LBrace,
  TreeTokenKind_RBrace,

  TreeTokenKind_Plus,
  TreeTokenKind_Minus,
  TreeTokenKind_Star,
  TreeTokenKind_Slash,

  TreeTokenKind_Identifier,
  TreeTokenKind_Int_Lit,

  TreeTokenKind_Return,
  TreeTokenKind_If,
  TreeTokenKind_Else,


  TreeTokenKind_EOF,
  TreeTOkenKind_COUNT,
};

typedef struct TreeToken TreeToken;
struct TreeToken {
  TreeTokenKind kind;
  U32 start, end;
};


extern char *tree_token_kind_strings[];
B32 tree_tokenizer_init(void);
U32 tree_hash_keyword(String str);
String string_from_source(Byte *src, U32 start, U32 end);
TreeToken *tree_tokenize(Arena *arena, U32 *tokencount, Byte *src, U32 srclen);
void print_tokens(TreeToken *tokens, U32 count, Byte *src);
#endif
