#ifndef _TOKENIZER
#define _TOKENIZER

#include <base/base_inc.h>

typedef U32 TokenKind;
enum {
  TokenKind_Invalid,
  TokenKind_NewLine,

  TokenKind_Int,

  TokenKind_Equals,
  TokenKind_SemiColon,
  TokenKind_Comma,

  TokenKind_LParen,
  TokenKind_RParen,
  TokenKind_LBrace,
  TokenKind_RBrace,

  TokenKind_Plus,
  TokenKind_Minus,
  TokenKind_Star,
  TokenKind_Slash,
  TokenKind_Percent,

  TokenKind_LogicNot,
  TokenKind_LogicEqual,
  TokenKind_LogicNotEqual,
  TokenKind_LogicGreaterThan,
  TokenKind_LogicGreaterEqual,
  TokenKind_LogicLesserThan,
  TokenKind_LogicLesserEqual,

  TokenKind_Identifier,
  TokenKind_IntLit,

  TokenKind_Return,
  TokenKind_If,
  TokenKind_Else,
  TokenKind_While,

  TokenKind_Struct,
  TokenKind_Fn,

  TokenKind_EOF,
  TokenKind_COUNT,
};

typedef struct Token Token;
struct Token {
  TokenKind kind;
  U32 start, end;
};



extern char *token_kind_strings[];
B32 tokenizer_init(void);
U32 hash_keyword(String8 str);
String8 str8_tok(String8 src, Token tok);
Token *tokenize(Arena *arena, U32 *tokencount, String8 src_string);
void print_tokens(Token *tokens, U32 count, String8 src);
#endif
