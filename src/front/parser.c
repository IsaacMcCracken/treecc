#include <front/front.h>
#include <stdarg.h>


void parse_stmt(Parser *p, SeaFunctionGraph *fn);
void parse_block(Parser *p, SeaFunctionGraph *fn);
SeaType *parse_type(Parser *p);

Token current_token(Parser *p) {
    return p->tokens[p->curr];
}

Token peek_token_n(Parser *p, U32 n) {
    U32 idx = Min(p->tok_count - 1, p->curr + n);

    return p->tokens[idx];
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
        case TokenKind_Percent:
            return 13;
        default:
            return -1;
    }
}

void skip_newlines(Parser *p) {
    Token tok = current_token(p);
    while (tok.kind == TokenKind_NewLine) {
        p->line += 1;
        advance_token(p);
        tok = current_token(p);
    }
}

String8 parser_get_curr_line_str8(Parser *p) {
    // start
    S32 curr = p->curr;
    while (curr > 0 && p->tokens[curr].kind != TokenKind_NewLine) {
        curr -= 1;
    }

    if (curr != 0) curr += 1;

    Token start_tok = p->tokens[curr];

    curr = p->curr;
    while (curr < p->tok_count && p->tokens[curr].kind != TokenKind_NewLine) {
        curr += 1;
    }
    curr -= 1;
    Token end_tok = p->tokens[curr];

    String8 str = (String8){
        p->src.str + start_tok.start,
        end_tok.end - start_tok.start,
    };

    return str;
}

void parser_error(Parser *p, const char *fmt, ...) {
    fprintf(stderr, "%.*s: line:%lu \x1b[31mError\x1b[0m: ", str8_varg(p->filename), p->line);

    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);


    va_end(args);

    String8 line_str = parser_get_curr_line_str8(p);
    fprintf(stderr, "\n\t%lu |\t%.*s\n", p->line, str8_varg(line_str));
}

SeaType *parse_type(Parser *p) {
    skip_newlines(p);

    Token tok = current_token(p);

    SeaType *t = 0;
    switch (tok.kind) {
        case TokenKind_Int: {
            advance_token(p);
            return &sea_type_S64;
        } break;

        default: {
            // TODO better errors
            fprintf(stderr, "Could not parse type '%.*s'", str8_varg(token_string(p, tok)));
            advance_token(p);
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

        case TokenKind_Identifier: {
            String8 name = token_string(p, tok);
            SeaNode *var = sea_scope_lookup_symbol(&p->m, name);
            if (!var) {
                parser_error(p, "in the expression the symbol '%.*s' is not defined.", str8_varg(name));
            }
            n = var;
        } break;

        case TokenKind_Minus: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_op(fn, SeaNodeKind_NegateI, input);
        } break;

        case TokenKind_LogicNot: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_op(fn, SeaNodeKind_Not, input);
        } break;

        default: {
            String8 str = token_string(p, tok);
            parser_error(p, "Error '%.*s' is not a valid urnary (%d).\n", str8_varg(str), tok.kind);
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
            case TokenKind_Percent: opkind = SeaNodeKind_ModI; break;
            case TokenKind_LogicEqual: opkind  = SeaNodeKind_EqualI; break;
            case TokenKind_LogicNotEqual: opkind  = SeaNodeKind_NotEqualI; break;
            case TokenKind_LogicGreaterThan: opkind  = SeaNodeKind_GreaterThanI; break;
            case TokenKind_LogicGreaterEqual: opkind  = SeaNodeKind_GreaterEqualI; break;
            case TokenKind_LogicLesserThan: opkind  = SeaNodeKind_LesserThanI; break;
            case TokenKind_LogicLesserEqual: opkind  = SeaNodeKind_LesserEqualI; break;
            default:
                //emit error
                String8 name = token_string(p, op);
                parser_error(p, "Expected a binary operator got %.*s", str8_varg(name));
                break;
        }

        SeaNode *opnode = sea_create_bin_op(fn, opkind, lhs, rhs);
        lhs = opnode;
    }

    return lhs;
}

SeaNode *parse_expr(Parser *p, SeaFunctionGraph *fn) {
    SeaNode *lhs = parse_urnary(p, fn);
    return parse_bin_expr(p, fn, lhs, 0);
}

void parse_if(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);

    Token tok = current_token(p);
    if (tok.kind != TokenKind_LParen) {
        parser_error(p, "expected a '('.");
    }

    advance_token(p);
    SeaNode *expr = parse_expr(p, fn);

    tok = current_token(p);
    if (tok.kind != TokenKind_RParen) {
        parser_error(p, "expected a ')'.");
    }


    // TODO find out why this returns 0
    SeaNode *prev_ctrl = p->m.curr->inputs[0];

    SeaNode *ifnode =   sea_create_if(fn, prev_ctrl, expr);
    sea_node_keep(fn, ifnode);

    SeaNode *fnode =    sea_create_proj(fn, ifnode, 0);

    sea_node_unkeep(fn, ifnode);
    SeaNode *tnode =    sea_create_proj(fn, ifnode, 1);

    SeaNode *tscope = p->m.curr;
    SeaNode *fscope = sea_duplicate_scope(fn, &p->m, 0);

    sea_scope_update_symbol(fn, &p->m, CTRL_STR, tnode);

    advance_token(p);
    skip_newlines(p);

    tok = current_token(p);
    if (tok.kind == TokenKind_LBrace) {
        parse_block(p, fn);
    }

    // Parse the false case
    p->m.curr = fscope;
    sea_scope_update_symbol(fn, &p->m, CTRL_STR, fnode);

    skip_newlines(p);
    tok = current_token(p);
    if (tok.kind == TokenKind_Else) {
        advance_token(p);
        tok = current_token(p);
        if (tok.kind == TokenKind_LBrace) {
            parse_block(p, fn);
        } else if (tok.kind == TokenKind_If) {
            parse_if(p, fn);
        } else {
            String8 name = token_string(p, tok);
            parser_error(p, "Expected 'if' or '{' got %.*s.", str8_varg(name));
        }
    }

    SeaNode *region = sea_merge_scopes(fn, &p->m, tscope);
    sea_scope_update_symbol(fn, &p->m, CTRL_STR, region);
}


void parse_while(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);

    // Create Loop and add Prev Ctrl as the entry set the new control to loop
    SeaNode *prev_ctrl = sea_scope_lookup_symbol(&p->m, CTRL_STR);
    SeaNode *loop = sea_create_loop(fn, prev_ctrl);
    sea_scope_update_symbol(fn, &p->m, CTRL_STR, loop);

    // // duplicate the scope with phis with as second input null
    SeaNode *head_scope = p->m.curr;
    p->m.curr = sea_duplicate_scope(fn, &p->m, 1);

    Token tok = current_token(p);
    if (tok.kind != TokenKind_LParen) {
        parser_error(p, "expected a '('.");
    }

    advance_token(p);
    SeaNode *expr = parse_expr(p, fn);

    tok = current_token(p);
    if (tok.kind != TokenKind_RParen) {
        parser_error(p, "expected a ')'.");
    }


    SeaNode *ifnode =   sea_create_if(fn, loop, expr);
    sea_node_keep(fn, ifnode);
    SeaNode *fnode =    sea_create_proj(fn, ifnode, 0);
    sea_node_unkeep(fn, ifnode);
    SeaNode *tnode =    sea_create_proj(fn, ifnode, 1);
    sea_scope_insert_symbol(fn, &p->m, CTRL_STR, tnode);


    SeaNode *back_scope = p->m.curr;
    SeaNode *exit_scope = sea_duplicate_scope(fn, &p->m, 0);
    p->m.curr = exit_scope;
    sea_scope_insert_symbol(fn, &p->m, CTRL_STR, fnode);

    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);

    if (tok.kind == TokenKind_LBrace) {
        parse_block(p, fn);
    }

    sea_scope_end_loop(fn, &p->m, head_scope, back_scope, exit_scope);

}

void parse_local_decl(Parser *p, SeaFunctionGraph *fn) {
    skip_newlines(p);
    SeaType *t = parse_type(p);
    Token tok = current_token(p);
    String8 name = token_string(p, tok);

    advance_token(p);
    tok = current_token(p);

    if (tok.kind == TokenKind_Equals) {
        advance_token(p);
        SeaNode *expr = parse_expr(p, fn);
        // TODO Replace with a real function
        // if (t->kind !=  expr->type->kind) {
        //     // Error
        //     fprintf(stderr, "Error: Type Mismatch");
        // }

        sea_scope_insert_symbol(fn, &p->m, name, expr);
    } else {
        // TODO top

    }

}

void parse_stmt(Parser *p, SeaFunctionGraph *fn) {
    Token tok = current_token(p);
    switch (tok.kind) {
        case TokenKind_LBrace: {
            parse_block(p, fn);
        } break;
        case TokenKind_Return: {
            advance_token(p);
            SeaNode *expr = parse_expr(p, fn);
            SeaNode *result = sea_peephole(fn, expr);
            // TODO add control stuff (FIXXX)
            if (result->kind == SeaNodeKind_ConstInt) {
                printf("%.*s() = %lld\n", str8_varg(fn->proto.name), expr->vint);
            } else {
                printf("%.*s() failed(%d)\n", str8_varg(fn->proto.name), (int)expr->kind);

            }
            SeaNode *ret = sea_create_return(fn, fn->start, expr);
        } break;
        case TokenKind_Identifier: {
            // TODO see if its a user defined type
            String8 name = token_string(p, tok);
            SeaNode *n = sea_scope_lookup_symbol(&p->m, name);
            if (!n) {
                String8 name = token_string(p, tok);
                parser_error(p, "symbol '%.*s' is not defined in scope.", str8_varg(name));
            }

            advance_token(p);
            tok = current_token(p);
            if (tok.kind == TokenKind_Equals) {
                advance_token(p);
                SeaNode *expr = parse_expr(p, fn);
                if (n) sea_scope_update_symbol(fn, &p->m, name, expr);
            }

        } break;
        case TokenKind_If: {
            parse_if(p, fn);
        } break;

        case TokenKind_While: {
            parse_while(p, fn);
        } break;

        case TokenKind_Int: {
            parse_local_decl(p, fn);
        } break;
        default: {
            // TODO error
            String8 name = token_string(p, tok);
            fprintf(stderr, "Error: unkown token '%.*s' while parsing statement.\n", str8_varg(name));
            advance_token(p);
        } break;
    }
}


void parse_block(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);
    skip_newlines(p);

    sea_push_scope(&p->m);



    Token tok = current_token(p);
    while ((p->curr < p->tok_count) && (tok.kind != TokenKind_RBrace)) {
        parse_stmt(p, fn);
        skip_newlines(p);
        tok = current_token(p);
    }

    advance_token(p);
    sea_pop_scope(&p->m);
}


void parse_func(Parser *p) {
    advance_token(p);
    SeaType *return_type = parse_type(p);

    Token name_tok = current_token(p);

    if (name_tok.kind != TokenKind_Identifier) {
        // TODO Error
    }

    String8 name = token_string(p, name_tok);
    advance_token(p);

    printf("Parsing: %.*s\n", str8_varg(name));

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
        SeaType *t = parse_type(p);
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
        Temp temp = temp_begin(p->m.arena);
        SeaNode *scope = sea_create_scope(&p->m, 128);
        p->m.curr = scope;

        SeaFunctionGraph *fn = sea_add_function(p->mod, &p->m, proto);
        parse_block(p, fn);

        p->m.curr = 0;
        p->m.cellpool = 0;
        p->m.scopepool = 0;
        temp_end(temp);

        printf("allocated %d bytes %d unused.\n", fn->arena->pos, fn->deadspace);
    } else {
        sea_add_function_symbol(p->mod, proto);
    }
}


void parse_decl(Parser *p) {


    Token tok = current_token(p);
    switch (tok.kind) {
        case TokenKind_Fn: {
        default: {
            parse_func(p);
        } break;
            String8 str = token_string(p, tok);
            printf("Error: Did not expect '%.*s' (%d) in %.*s.\n", str8_varg(str), tok.kind, str8_varg(p->filename));
        } break;
    }
}

void parse_decls(Parser *p) {
    while (p->curr < p->tok_count) {
        skip_newlines(p);
        Token tok = current_token(p);
        if (tok.kind == TokenKind_EOF) {
            return;
        }

        parse_decl(p);
    }
}



void module_add_file_and_parse(Module *mod, String8 filename) {
    Arena *arena = arena_alloc(); // TODO BETTER RESERVE SIZE
    Temp scratch = scratch_begin(0, 0);

    String8List l = {0};
    str8_list_push(scratch.arena, &l, mod->path);
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
        .mod = &mod->m,
        .arena = arena,
        .m = (SeaScopeManager){
            .arena = arena,
            .default_cap = 61,
        },
        .filename = filename,
        .src = src,
        .tokens = tokens,
        .tok_count = tok_count,
        .curr = 0,
    };

    parse_decls(&p);
}
