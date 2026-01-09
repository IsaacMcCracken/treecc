#include "tokenizer.h"
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

U32 hash_dbj2(Byte *data, U64 len) {
    U32 hash = 5382;
    for (U32 i = 0; i < len; i++) {
        Byte c = data[i];
        hash = ((hash<<5) + hash) + c;
    }

    return hash;
}

U32 hash_keyword(String str) {
    U32 hash = 0;
    switch (str.len) {
        case 0: assert(0);
        case 1: case 2: case 3:
            for (U32 i = 0; i < str.len; i++) {
                hash = (hash << 8) | str.str[i];
            }
            return hash;
    }

    hash |= (U32)str.str[0] << 24;
    hash |= (U32)str.str[1] << 16;
    hash |= (U32)str.str[str.len - 2] << 8;
    hash |= (U32)str.str[str.len - 1];

    return hash;
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

String string_from_source(Byte *src, U32 start, U32 end) {
    return (String){
        (char*)src + start,
        end - start
    };
}

void append_token(Arena *arena, TokenKind kind, U32 start, U32 end, U32 *count) {
    Token *tok = arena_push(arena, Token);
    tok->kind = kind;
    tok->start = start;
    tok->end = end;


    *count += 1;
}

void append_keyword_or_identifier(Arena *arena, Byte *src, U32 start, U32 end, U32 *tokencount) {


    String tok_str = string_from_source(src, start, end);
    U32 hash = hash_keyword(tok_str);
    U32 hashv = hash%KEYWORD_MAP_SIZE;

    TokenKind kind = keyword_map[hashv];
    if (string_cmp(tok_str, keywords[kind]) != 0) kind = TokenKind_Invalid;
    if (kind == TokenKind_Invalid) kind = TokenKind_Identifier;

    append_token(arena, kind, start, end, tokencount);
}



Token *tokenize(
    Arena *arena,
    U32 *tokencount,
    Byte *src,
    U32 srclen
) {
    U32 count = 0;
    U32 curr = 0, prev = 0;

    Token *tokens = arena_get_current(arena);
    while (curr < srclen) {
        prev = curr;
        Byte ch = src[curr];
        if (is_identifier_begin_rune(ch)) {
            while (is_identifier_rune(ch)) {
                curr += 1;
                ch = src[curr];
            }

            append_keyword_or_identifier(arena, src, prev, curr, &count);
        } else if (is_number_begin_rune(ch)) {

            while (is_number_rune(ch)) {
                curr += 1;
                ch = src[curr];
                // putchar(ch);
            }

            append_token(arena, TokenKind_Int_Lit, prev, curr, &count);
        } else {
            curr += 1;
            switch (ch) {
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
                    append_token(arena, TokenKind_Slash, prev, curr, &count);
                    break;
                case ';':
                    append_token(arena, TokenKind_SemiColon, prev, curr, &count);
                    break;
                case ',':
                    append_token(arena, TokenKind_Comma, prev, curr, &count);
                    break;
                case '=': {
                    // TODO bound check
                    if (src[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_Equals, prev, curr, &count);
                    }

                } break;

                case '!': {
                    // TODO bound check
                    if (src[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicNotEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicNot, prev, curr, &count);
                    }

                } break;

                case '>': {
                    // TODO bound check
                    if (src[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicGreaterEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicGreaterThan, prev, curr, &count);
                    }

                } break;

                case '<': {
                    // TODO bound check
                    if (src[curr] == '=') {
                        curr += 1;
                        append_token(arena, TokenKind_LogicLesserEqual, prev, curr, &count);
                    } else {
                        append_token(arena, TokenKind_LogicLesserThan, prev, curr, &count);
                    }

                } break;
            }
        }
    }

    *tokencount = count;
    return tokens;
}

void print_tokens(Token *tokens, U32 count, Byte *src) {
    for (U32 i = 0; i < count; i++) {
        Token tok = tokens[i];
        String str = string_from_source(src, tok.start, tok.end);
        printf("'%.*s' = %d\n", (int)str.len, str.str, tok.kind);
    }
}
