#include <front/front.h>



Parser parser_begin(String filename) {
    Arena *arena = arena_init(MEGABYTE(64), ArenaFlag_Chainable);
    String src = os_read_entire(arena, filename);
    U32 tok_count = 0;
    Token *tokens = tokenize(arena, &tok_count, (Byte*)src.ptr, src.len);

    Parser p = (Parser){
        .arena = arena,
        .filename = filename,
        .src = src,
        .tokens = tokens,
        .tok_count = tok_count,
        .curr = 0,
    };

    return p;
}

Token current_token(Parser *p) {
    return p->tokens[p->curr];
}

Token peek_token_n(Parser *p, U32 n) {
    if (p->curr + n < p->tok_count) {
        return p->->tokens[p->curr + n];
    }

    return (Token){.kind = TokenKind_EOF};
}

Token peek_token_next(Parser *p) {
    return tree_peek_n(p, 1);
}

void advance_token(Parser *p) {
    p->curr += 1;
}

SeaDataType *parse_type(Parser *p) {
    // TODO make unique types;
    Token tok = current_token(p);
    SeaDataType *type = 0;
    switch (tok.kind) {
        case TokenKind_Int: {
            type = arena_push(p->arena);
            type->kind = SeaDataType_I32;
        } break;
        default: break;
    }

    advance_token(p);
    return type;
}

void parse_func(Parser *p) {
    Token tok = current_token(p);

}

void parse_decl(Parser *p) {
    Token tok = current_token(p);

    switch (tok.kind) {
        case TokenKind_Fn: {
            advance_token(p);
            parse_func(p);
        } break;
    }
}
