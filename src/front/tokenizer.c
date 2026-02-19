#include "tokenizer.h"
#include <assert.h>
#include "gen/keywords.c"

char *token_kind_strings[] = {
    "(nil)",
    "Int",
    "Equals",
    "SemiColon",
    "Comma",
    "LParen",
    "RParen",
    "LBrace",
    "RBrace",
    "Plus",
    "Minus",
    "Star",
    "Slash",
    "Identifier",
    "Int_Lit",
    "Return",
    "EOF",
};

B32 tokenizer_init(void) {
    init_token_maps();
    return 1;
}



U32 hash_keyword(String8 str) {
    U32 hash = 0;
    switch (str.size) {
        case 0: assert(0);
        case 1: case 2: case 3:
            for (U32 i = 0; i < str.size; i++) {
                hash = (hash << 8) | str.str[i];
            }
            return hash;
    }

    hash |= (U32)str.str[0] << 24;
    hash |= (U32)str.str[1] << 16;
    hash |= (U32)str.str[str.size - 2] << 8;
    hash |= (U32)str.str[str.size - 1];

    return hash;
}

B32 is_whitespace_rune(U32 rune) {
    if (rune == ' ' || rune == '\t' || rune == '\r') return 1;
    return 0;
}

B32 is_identifier_begin_rune(U32 rune) {
    if (rune >= 'a' && rune <= 'z') return 1;
    if (rune >= 'A' && rune <= 'Z') return 1;
    if (rune == '_') return 1;
    return 0;
}

B32 is_number_begin_rune(U32 rune) {
    if (rune >= '0' && rune <= '9') return 1;
    return 0;
}


B32 is_number_rune(U32 rune) {
    if (rune >= '0' && rune <= '9') return 1;
    return 0;
}

B32 is_identifier_rune(U32 rune) {
    if (rune >= 'a' && rune <= 'z') return 1;
    if (rune >= 'A' && rune <= 'Z') return 1;
    if (rune >= '0' && rune <= '9') return 1;
    if (rune == '_') return 1;
    return 0;
}

String8 string_from_source(String8 src, U32 start, U32 end) {
    return (String8){
        (U8*)src.str + start,
        end-start,
    };
}
String8 str8_tok(String8 src, Token tok) {
    return string_from_source(src, tok.start, tok.end);
}

void append_token(Arena *arena, TokenKind kind, U32 start, U32 end, U32 *count) {
    Token *tok = push_item(arena, Token);
    tok->kind = kind;
    tok->start = start;
    tok->end = end;

    if (kind == 0) {
        fprintf(stderr, "WTFART");
    }

    *count += 1;
}

void append_keyword_or_identifier(Arena *arena, String8 src, U32 start, U32 end, U32 *tokencount) {


    String8 tok_str = string_from_source(src, start, end);
    U32 hash = hash_keyword(tok_str);
    U32 hashv = hash%KEYWORD_MAP_SIZE;

    TokenKind kind = keyword_map[hashv];
    if (!str8_match(tok_str, keywords[kind], 0)) kind = TokenKind_Invalid;
    if (kind == TokenKind_Invalid) kind = TokenKind_Identifier;


    append_token(arena, kind, start, end, tokencount);


}

Token *tokenize(Arena *arena, U32 *token_count, String8 src) {
    arena_align_forward(arena, AlignOf(Token));
    Token *tokens = arena_pos_ptr(arena);
    U32 curr = 0, prev = 0, count = 0;
    while (curr < src.size) {
        prev = curr;
        U8 ch = src.str[curr];
        if (is_identifier_begin_rune(ch)) {
            while (is_identifier_rune(ch)) {
                curr += 1;
                ch = src.str[curr];
            }

            append_keyword_or_identifier(arena, src, prev, curr, &count);
        } else if (is_number_begin_rune(ch)) {

            while (is_number_rune(ch)) {
                curr += 1;
                ch = src.str[curr];
                // putchar(ch);
            }

            append_token(arena, TokenKind_IntLit, prev, curr, &count);
        } else {
            curr += 1;
            switch (ch) {
                case '\n':
                    append_token(arena, TokenKind_NewLine, prev, curr, &count);
                    break;
                case '(':
                    append_token(arena, TokenKind_LParen, prev, curr, &count);
                    break;
                case ')':
                    append_token(arena, TokenKind_RParen, prev, curr, &count);
                    break;
                case '{':
                    append_token(arena, TokenKind_LBrace, prev, curr, &count);
                    break;
                case '}':
                    append_token(arena, TokenKind_RBrace, prev, curr, &count);
                    break;
                case '+':
                    append_token(arena, TokenKind_Plus, prev, curr, &count);
                    break;
                case '-':
                    append_token(arena, TokenKind_Minus, prev, curr, &count);
                    break;
                case '*':
                    append_token(arena, TokenKind_Star, prev, curr, &count);
                    break;
                case '/':
                    if (src.str[curr] == '/') {
                        while (curr < src.size && src.str[curr] != '\n') {
                            curr += 1;
                        }
                    } else {
                        append_token(arena, TokenKind_Slash, prev, curr, &count);
                    }
                    break;
                case '%':
                    append_token(arena, TokenKind_Percent, prev, curr, &count);
                    break;
                case ';':
                    append_token(arena, TokenKind_SemiColon, prev, curr, &count);
                    break;
                case ',':
                    append_token(arena, TokenKind_Comma, prev, curr, &count);
                    break;
                case '=': {
                    // TODO bound check
                    if (src.str[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_Equals, prev, curr, &count);
                    }

                } break;

                case '!': {
                    // TODO bound check
                    if (src.str[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicNotEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicNot, prev, curr, &count);
                    }

                } break;

                case '>': {
                    // TODO bound check
                    if (src.str[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicGreaterEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicGreaterThan, prev, curr, &count);
                    }

                } break;

                case '<': {
                    // TODO bound check
                    if (src.str[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicLesserEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicLesserThan, prev, curr, &count);
                    }

                } break;


                case ' ':
                case '\t':
                case '\r': {
                    // DO NOTHING FOR WHITE SPACE
                } break;

                default: {
                    fprintf(stderr, "Error: could not recognize character '%c'", ch);
                    assert(0);
                }
            }
        }
    }


    append_token(arena, TokenKind_EOF, src.size, src.size, &count);

    *token_count = count;
    return tokens;
}


void print_tokens(Token *tokens, U32 count, String8 src) {
    for (U32 i = 0; i < count; i++) {
        Token tok = tokens[i];
        String8 str = str8_tok(src, tok);
        printf("'%.*s' = %d\n", (int)str.size, str.str, tok.kind);
    }
}
