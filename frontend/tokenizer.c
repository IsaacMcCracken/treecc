#include "tokenizer.h"
#include "gen/keywords.c"

B32 tree_tokenizer_init(void) {
    tree_init_token_maps();
    return 1;
}

U32 tree_hash_dbj2(Byte *data, U64 len) {
    U32 hash = 5382;
    for (U32 i = 0; i < len; i++) {
        Byte c = data[i];
        hash = ((hash<<5) + hash) + c;
    }

    return hash;
}

U32 tree_hash_string(String str) {
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

B32 tree_is_identifier_begin_rune(U32 rune) {
    if (rune >= 'a' && rune <= 'z') return 1;
    if (rune >= 'A' && rune <= 'Z') return 1;
    if (rune == '_') return 1;
    return 0;
}

B32 tree_is_number_begin_rune(U32 rune) {
    if (rune >= '0' && rune <= '9') return 1;
    return 0;
}


B32 tree_is_number_rune(U32 rune) {
    if (rune >= '0' && rune <= '9') return 1;
    return 0;
}

B32 tree_is_identifier_rune(U32 rune) {
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

void tree_append_token(Arena *arena, TreeTokenKind kind, U32 start, U32 end, U32 *count) {
    TreeToken *tok = arena_push(arena, TreeToken);
    tok->kind = kind;
    tok->start = start;
    tok->end = end;


    *count += 1;
}

void tree_append_keyword_or_identifier(Arena *arena, Byte *src, U32 start, U32 end, U32 *tokencount) {


    String tok_str = string_from_source(src, start, end);
    U32 hash = tree_hash_string(tok_str);
    U32 hashv = hash%KEYWORD_MAP_SIZE;

    TreeTokenKind kind = keyword_map[hashv];
    if (string_cmp(tok_str, keywords[kind]) != 0) kind = TreeTokenKind_Invalid;
    if (kind == TreeTokenKind_Invalid) kind = TreeTokenKind_Identifier;

    tree_append_token(arena, kind, start, end, tokencount);
}



TreeToken *tree_tokenize(
    Arena *arena,
    U32 *tokencount,
    Byte *src,
    U32 srclen
) {
    U32 count = 0;
    U32 curr = 0, prev = 0;

    TreeToken *tokens = arena_get_current(arena);
    while (curr < srclen) {
        prev = curr;
        Byte ch = src[curr];
        if (tree_is_identifier_begin_rune(ch)) {
            while (tree_is_identifier_rune(ch)) {
                curr += 1;
                ch = src[curr];
            }

            tree_append_keyword_or_identifier(arena, src, prev, curr, &count);
        } else if (tree_is_number_begin_rune(ch)) {

            while (tree_is_number_rune(ch)) {
                curr += 1;
                ch = src[curr];
                // putchar(ch);
            }

            tree_append_token(arena, TreeTokenKind_Int_Lit, prev, curr, &count);
        } else {
            curr += 1;
            switch (ch) {
                case '+':
                    tree_append_token(arena, TreeTokenKind_Plus, prev, curr, &count);
                    break;
                case '-':
                    tree_append_token(arena, TreeTokenKind_Minus, prev, curr, &count);
                    break;
                case '*':
                    tree_append_token(arena, TreeTokenKind_Star, prev, curr, &count);
                    break;
                case '/':
                    tree_append_token(arena, TreeTokenKind_Slash, prev, curr, &count);
                    break;
                case ';':
                    tree_append_token(arena, TreeTokenKind_Semi_Colon, prev, curr, &count);
                    break;
                case '=':
                    tree_append_token(arena, TreeTokenKind_Equals, prev, curr, &count);
                    break;
            }
        }

    }

    *tokencount = count;
    return tokens;
}

void print_tokens(TreeToken *tokens, U32 count, Byte *src) {
    for (U32 i = 0; i < count; i++) {
        TreeToken tok = tokens[i];
        String str = string_from_source(src, tok.start, tok.end);
        printf("'%.*s' = %d\n", (int)str.len, str.str, tok.kind);
    }
}
