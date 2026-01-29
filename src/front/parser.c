#include <front/front.h>

Token current_token(Parser *p) {
    return p->tokens[p->curr];
}

Token peek_token_n(Parser *p, U32 n) {
    if (p->curr + n < p->tok_count) {
        return p->tokens[p->curr + n];
    }

    return (Token){.kind = TokenKind_EOF};
}

Token peek_token_next(Parser *p) {
    return peek_token_n(p, 1);
}

void advance_token(Parser *p) {
    p->curr += 1;
}

String8 token_string(Parser *p, Token tok) {
    str8_tok(p->src, tok);
}

S32 operatator_precedence(Token tok) {
    switch (tok.kind) {
        // TODO Logical || value = 3
        // TODO Logical && value = 4
        // TODO Bitwise | value = 5
        // TODO Bitwise ^ value = 6
        // TODO Bitwise & value = 7
        case TokenKind_LogicEqual:
        case TokenKind_LogicNotEqual:
            return 8;
        case TokenKind_LogicGreaterThan:
        case TokenKind_LogicGreaterEqual:
        case TokenKind_LogicLesserThan:
        case TokenKind_LogicLesserEqual:
            return 9;
        // TODO Bit shit (value = 10)
        case TokenKind_Minus:
        case TokenKind_Plus:
            return 11;
        case TokenKind_Star:
        case TokenKind_Slash:
            return 13;
        default:
            return -1;
    }
}

void skip_newlines(Parser *p) {
    Token tok = current_token(p);
    while (tok.kind == TokenKind_NewLine) {
        advance_token(p);
        tok = current_token(p);
    }
}


SeaDataType *parse_type(Parser *p) {
    local_persist SeaDataType int32t = (SeaDataType){SeaDataKind_I32};

    skip_newlines(p);

    Token tok = current_token(p);

    SeaDataType *t = 0;
    switch (tok.kind) {
        case TokenKind_Int: {
            advance_token(p);
            return &int32t;
        } break;

        default: {
            // TODO better errors
            fprintf(stderr, "Could not parse type");
        } break;
    }

    return t;
}

SeaNode *parse_urnary(Parser *p, SeaFunctionGraph *fn) {
    Token tok = current_token(p);
    advance_token(p);

    SeaNode *n = 0;

    switch (tok.kind) {
        case TokenKind_IntLit: {
            String8 intstr =  token_string(p, tok);
            S64 v = s64_from_str8(intstr, 10);
            n = sea_create_const_int(fn, v);
        } break;

        case TokenKind_Minus: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_expr(fn, SeaNodeKind_NegateI, input);
        } break;

        case TokenKind_LogicNot: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_expr(fn, SeaNodeKind_Not, input);
        } break;

        default: {
            // TODO error
            String8 str = token_string(p, tok);
            fprintf(stderr, "Error '%.*s' is not a valid urnary (%d).\n", str8_varg(str), tok.kind);
        } break;
    }

    return n;
}

SeaNode *parse_bin_expr(
    Parser *p,
    SeaFunctionGraph *fn,
    SeaNode *lhs,
    S32 precedence
) {
    Token lookahead = current_token(p);
    while (operatator_precedence(lookahead) >= precedence) {
        Token op = lookahead;
        advance_token(p);

        SeaNode *rhs = parse_urnary(p, fn);
        lookahead = current_token(p);
        while (operatator_precedence(lookahead) > operatator_precedence(op)) {
            rhs = parse_bin_expr(p, fn, rhs, operatator_precedence(op) + 1);
            lookahead = peek_token_next(p);
        }

        SeaNodeKind opkind = 0;
        switch (op.kind) {
            case TokenKind_Plus:    opkind = SeaNodeKind_AddI; break;
            case TokenKind_Minus:   opkind = SeaNodeKind_SubI; break;
            case TokenKind_Star:    opkind = SeaNodeKind_MulI; break;
            case TokenKind_Slash:   opkind = SeaNodeKind_DivI; break;
            case TokenKind_LogicEqual: opkind  = SeaNodeKind_EqualI; break;
            case TokenKind_LogicNotEqual: opkind  = SeaNodeKind_NotEqualI; break;
            case TokenKind_LogicGreaterThan: opkind  = SeaNodeKind_GreaterThanI; break;
            case TokenKind_LogicGreaterEqual: opkind  = SeaNodeKind_GreaterEqualI; break;
            case TokenKind_LogicLesserThan: opkind  = SeaNodeKind_LesserThanI; break;
            case TokenKind_LogicLesserEqual: opkind  = SeaNodeKind_LesserEqualI; break;
            default:
                //emit error
                fprintf(stderr, "Error: expected a binary operator.\n");
                break;
        }

        SeaNode *opnode = sea_create_binary_expr(fn, opkind, lhs, rhs);
        lhs = opnode;
    }

    return lhs;
}

SeaNode *parse_expr(Parser *p, SeaFunctionGraph *fn) {
    SeaNode *lhs = parse_urnary(p, fn);
    return parse_bin_expr(p, fn, lhs, 0);
}

void parse_stmt(Parser *p, SeaFunctionGraph *fn) {
    Token tok = current_token(p);
    advance_token(p);
    switch (tok.kind) {
        case TokenKind_Return: {
            SeaNode *expr = parse_expr(p, fn);
            // TODO add control stuff (FIXXX)
            printf("%.*s() = %lld\n", str8_varg(fn->proto.name), expr->vint);
            SeaNode *ret = sea_create_return(fn, fn->start, expr);
        } break;

        default: {
            // TODO error
        }
    }
}


void parse_block(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);
    skip_newlines(p);

    Token tok = current_token(p);
    while ((p->curr < p->tok_count) && (tok.kind != TokenKind_RBrace)) {
        parse_stmt(p, fn);
        skip_newlines(p);
        tok = current_token(p);

        String8 trk = token_string(p, tok);
        // printf("%.*s\n", trk);
    }

    advance_token(p);
}

void parse_func(Parser *p) {
    advance_token(p);
    SeaDataType *return_type = parse_type(p);

    Token name_tok = current_token(p);

    if (name_tok.kind != TokenKind_Identifier) {
        // TODO Error
    }

    String8 name = token_string(p, name_tok);
    advance_token(p);

    Token tok = current_token(p);
    if (tok.kind != TokenKind_LParen) {
    }

    advance_token(p);

    // Should be '(' right here
    tok = current_token(p);

    // TODO will cause memory bugs in future


    arena_align_forward(p->arena, AlignOf(SeaField));
    SeaField *fields = arena_pos_ptr(p->arena);
    U64 field_count = 0;

    while (tok.kind != TokenKind_RParen) {
        SeaDataType *t = parse_type(p);
        skip_newlines(p);
        tok = current_token(p);

        if (tok.kind != TokenKind_Identifier) {
            // TODO error
        }

        String8 name = token_string(p, tok);

        SeaField *field = push_item(p->arena, SeaField);
        field->name = name;
        field->type = t;
        field_count += 1;

        advance_token(p);
        skip_newlines(p);
        tok = current_token(p);

        if (tok.kind == TokenKind_Comma) {
            advance_token(p);
            continue;
        } else if (tok.kind == TokenKind_RParen) {
            break;
        } else {
            // TODO error
        }
    }


    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);

    SeaFunctionProto proto = (SeaFunctionProto) {
        .args = (SeaFieldArray) {
            .fields = fields,
            .count = field_count,
        },
        .name =  name,
    };

    if (tok.kind == TokenKind_LBrace) {
        SeaFunctionGraph *fn = sea_add_function(p->m, proto);
        parse_block(p, fn);
    } else {
        sea_add_function_symbol(p->m, proto);
    }
}

void parse_decl(Parser *p) {

    skip_newlines(p);

    Token tok = current_token(p);

    switch (tok.kind) {
        case TokenKind_Fn: {
            parse_func(p);
        } break;
        default: {
            String8 str = str8_tok(p->src, tok);
            printf("Error: Did not expect '%.*s' (%d) in %.*s.\n", str8_varg(str), tok.kind, str8_varg(p->filename));
        } break;
    }
}

void parse_decls(Parser *p) {
    while (p->curr < p->tok_count) {
        Token tok = current_token(p);
        if (tok.kind == TokenKind_EOF) {
            return;
        }
        parse_decl(p);
    }
}



void module_add_file_and_parse(Module *m, String8 filename) {
    Arena *arena = arena_alloc(); // TODO BETTER RESERVE SIZE
    Temp scratch = scratch_begin(0, 0);

    String8List l = {0};
    str8_list_push(scratch.arena, &l, m->path);
    str8_list_push(scratch.arena, &l, str8_lit("/"));
    str8_list_push(scratch.arena, &l, filename);


    String8 path = str8_list_join(scratch.arena, &l, 0);
    // printf("Parsing: \"%.*s\"\n", str8_varg(path));

    OS_Handle file = os_file_open(OS_AccessFlag_Read, path);
    scratch_end(scratch);

    String8 src = os_file_read_entire_file(arena, file);
    os_file_close(file);

    U32 tok_count = 0;
    Token *tokens = tokenize(arena, &tok_count, src);

    printf("Parsing: %.*s\n%.*s\n",str8_varg(filename), str8_varg(src));

    Parser p = (Parser){
        .m = &m->m,
        .arena = arena,
        .filename = filename,
        .src = src,
        .tokens = tokens,
        .tok_count = tok_count,
        .curr = 0,
    };

    parse_decl(&p);
}
