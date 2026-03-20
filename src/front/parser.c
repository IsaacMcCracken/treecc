#include <front/front.h>
#include <stdarg.h>


void parse_stmt(Parser *p, SeaFunctionGraph *fn);
void parse_block(Parser *p, SeaFunctionGraph *fn);
SeaType *parse_type(Parser *p);
SeaNode *parse_func_call_expr(Parser *p, SeaFunctionGraph *fn);
void parse_struct_expr(Parser *p, SeaFunctionGraph *fn, SeaType *t);

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
    fprintf(stderr, "\n\t%lu |\t%.*s\n", p->line + 1, str8_varg(line_str));
}

SeaType *parse_type(Parser *p) {
    skip_newlines(p);

    Token tok = current_token(p);

    SeaType *t = 0;
    switch (tok.kind) {
        case TokenKind_Int: {
            advance_token(p);
            t = &sea_type_S64;
        } break;

        case TokenKind_Identifier: {
            // SeaType *lookup
            String8 name = token_string(p, tok);
            SeaSymbolEntry *sym = sea_lookup_symbol(p->mod, name);
            if (sym) {
                advance_token(p);
                t = sym->type;
            }
        } break;

        case TokenKind_LBrack: {
            advance_token(p);
            tok = current_token(p);
            String8 str = token_string(p, tok);
            if (tok.kind != TokenKind_IntLit) {
                parser_error(p, "expected a number got '%.*s' note:(will have slices in future)", str8_varg(str));
            } else {
                U64 len = (U64)s64_from_str8(str, 10);
                advance_token(p);
                tok = current_token(p);
                if (tok.kind != TokenKind_RBrack) {
                    parser_error(p, "todo error message");
                }
                advance_token(p);
                SeaType *base = parse_type(p);
            }
        } break;


    }

    return t;
}

SeaNode *parse_urnary(Parser *p, SeaFunctionGraph *fn) {
    Token tok = current_token(p);

    SeaNode *n = 0;

    switch (tok.kind) {
        case TokenKind_IntLit: {
            String8 intstr =  token_string(p, tok);
            S64 v = s64_from_str8(intstr, 10);
            n = sea_create_const_int(fn, v);
            advance_token(p);
        } break;

        case TokenKind_Identifier: {
            String8 name = token_string(p, tok);
            SeaNode *var = sea_scope_lookup_symbol(&p->m, name);

            SeaSymbolEntry *sym = sea_lookup_symbol(p->mod, name);


            if (!var && !sym) {
                parser_error(p, "in the expression the symbol '%.*s' is not defined.", str8_varg(name));
            } else if (var) {
                n = var;
                advance_token(p);
            } else if (sym) {
                switch (sym->type->kind) {
                    case SeaLatticeKind_Func: {
                        parse_func_call_expr(p, fn);
                    } break;
                    case SeaLatticeKind_Struct: {
                        parse_struct_expr(p, fn, 0);
                    } break;
                }
            } else {
                parser_error(p, "nah you can't have a variable named after a symbol");
                // todo skip
            }
        } break;

        case TokenKind_Minus: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_op(fn, SeaNodeKind_NegI, input);
            advance_token(p);
        } break;

        case TokenKind_LogicNot: {
            advance_token(p);
            SeaNode *input = parse_urnary(p, fn);
            n = sea_create_urnary_op(fn, SeaNodeKind_Not, input);
            advance_token(p);
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
    SeaNode *expr = sea_peephole(fn, parse_expr(p, fn));

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

    sea_node_set_ctrl(fn, p->m.curr, tnode);

    advance_token(p);
    skip_newlines(p);

    tok = current_token(p);
    if (tok.kind == TokenKind_LBrace) {
        parse_block(p, fn);
    }
    tscope = p->m.curr; // lmao hacky fix
    // Parse the false case
    p->m.curr = fscope;
    sea_node_set_ctrl(fn, p->m.curr, fnode);

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
    fscope = p->m.curr;

    SeaNode *region = sea_merge_scopes(fn, &p->m, tscope);
    sea_node_set_ctrl(fn, p->m.curr, region);
}


void parse_while(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);

    // Create Loop and add Prev Ctrl as the entry set the new control to loop
    SeaNode *prev_ctrl = sea_node_get_ctrl(fn, p->m.curr);
    SeaNode *loop = sea_create_loop(fn, prev_ctrl);
    sea_node_set_ctrl(fn, p->m.curr, loop);

    // // duplicate the scope with phis with as second input null
    SeaNode *head_scope = p->m.curr;
    p->m.curr = sea_duplicate_scope(fn, &p->m, 1);

    Token tok = current_token(p);
    if (tok.kind != TokenKind_LParen) {
        parser_error(p, "expected a '('.");
    }

    advance_token(p);
    SeaNode *expr = sea_peephole(fn, parse_expr(p, fn));

    tok = current_token(p);
    if (tok.kind != TokenKind_RParen) {
        parser_error(p, "expected a ')'.");
    }


    SeaNode *ifnode =   sea_create_if(fn, loop, expr);
    sea_node_keep(fn, ifnode);
    SeaNode *fnode =    sea_create_proj(fn, ifnode, 0);
    sea_node_unkeep(fn, ifnode);
    SeaNode *tnode =    sea_create_proj(fn, ifnode, 1);


    SeaNode *body_scope = p->m.curr;
    sea_node_set_ctrl(fn, body_scope, tnode);
    SeaNode *exit_scope = sea_duplicate_scope(fn, &p->m, 0);
    sea_node_set_ctrl(fn, exit_scope, fnode);

    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);

    if (tok.kind == TokenKind_LBrace) {
        parse_block(p, fn);
    }
    body_scope = p->m.curr; // hacky shit but idk

    p->m.curr = exit_scope;

    sea_scope_end_loop(fn, &p->m, head_scope, body_scope, exit_scope);

    sea_scope_free(fn, &p->m, head_scope);
}



SeaNode *parse_return(Parser *p, SeaFunctionGraph *fn) {
    advance_token(p);
    SeaNode *expr = parse_expr(p, fn);
    expr = sea_peephole(fn, expr);
    SeaNode *ctrl = sea_node_get_ctrl(fn, p->m.curr);
    SeaNode *ret = sea_create_return(fn, ctrl, expr);
    // add return to the stop node
    sea_add_return(fn, ret);
    sea_node_set_ctrl(fn, p->m.curr, sea_create_dead_ctrl(fn));
    return ret;
}

SeaNode *parse_expr_stmt(Parser *p, SeaFunctionGraph *fn) {
    SeaNode *node = 0;
    // SeaType *type = parse_type(p, fn);
    SeaType *t = parse_type(p);
    Token tok = current_token(p);
    String8 var_name = token_string(p, tok);

    if (!t) {
        SeaNode *lookup = sea_scope_lookup_symbol(&p->m, var_name);
        if (!lookup) {
            parser_error(p, "\"%.*s\" is not defined", str8_varg(var_name));
        }
    } else {
        tok = current_token(p);
        String8 var_name = token_string(p, tok);

    }


    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);


    // for now always expect =
    if (tok.kind == TokenKind_Equals) {
        advance_token(p);
        SeaNode *expr = parse_expr(p, fn);
        node = sea_peephole(fn, expr);
        if (t) sea_scope_insert_symbol(fn, &p->m, var_name, node);
        else sea_scope_update_symbol(fn, &p->m, var_name, node);
    } else {
        String8 str = token_string(p, tok);
        parser_error(p, "expected '=' got %.*s (will default to 0 in future)", str8_varg(str));
    }

    return node;
}

void parse_stmt(Parser *p, SeaFunctionGraph *fn) {
    Token tok = current_token(p);
    switch (tok.kind) {
        case TokenKind_LBrace: {
            parse_block(p, fn);
        } break;
        case TokenKind_Return: {
            parse_return(p, fn);
        } break;
        case TokenKind_If: {
            parse_if(p, fn);
        } break;

        case TokenKind_While: {
            parse_while(p, fn);
        } break;

        default: {
            parse_expr_stmt(p, fn);
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
    sea_pop_scope(fn, &p->m);
}


SeaType *parse_struct(Parser *p) {
    advance_token(p);
    skip_newlines(p);
    Token name_tok = current_token(p);
    String8 struct_name = token_string(p, name_tok);



    if (name_tok.kind != TokenKind_Identifier) {
        parser_error(p, "Expected a name for struct got %.*s", str8_varg(struct_name));
    }

    advance_token(p);
    skip_newlines(p);
    Token tok = current_token(p);

    if (tok.kind != TokenKind_LBrace) {
        String8 str = token_string(p, tok);
        parser_error(p, "Expected a '{' got %.*s.", str8_varg(str));
    }


    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);

    arena_align_forward(p->arena, AlignOf(SeaField));
    SeaField *fields = arena_pos_ptr(p->arena);
    U64 field_count = 0;

    while (p->curr < p->tok_count && tok.kind != TokenKind_RBrace) {
        SeaType *type = parse_type(p);
        skip_newlines(p);
        tok = current_token(p);
        String8 name = token_string(p, tok);
        if (tok.kind != TokenKind_Identifier) {
            parser_error(p, "Expected a field name got %.*s.", str8_varg(name));
        }

        SeaField *field = push_item(p->arena, SeaField);
        field_count += 1;

        field->name = name;
        field->type = type;

        advance_token(p);
        tok = current_token(p);
        if (tok.kind != TokenKind_RBrace && tok.kind != TokenKind_Comma) {
            String8 str = token_string(p, tok);
            parser_error(p, "Expected a ',' or '}' got '%.*s'.", str8_varg(str));
        }

        advance_token(p);
        skip_newlines(p);
        tok = current_token(p);
    }
    advance_token(p);
    SeaFieldArray arr = {fields, field_count};
    SeaTypeStruct s = {struct_name, arr};
    sea_add_struct_symbol(p->mod, s);
}


void parse_func(Parser *p) {
    advance_token(p);
    SeaType *return_type = parse_type(p);

    Token name_tok = current_token(p);
    String8 name = token_string(p, name_tok);



    if (name_tok.kind != TokenKind_Identifier) {
        parser_error(p, "Expected a name for function got %.*s", str8_varg(name));
    }
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

        sea_begin_scope(&p->m);
        SeaFunctionGraph *fn = sea_add_function(p->mod, &p->m, proto);
        parse_block(p, fn);
        sea_end_scope(fn, &p->m);

        temp_end(temp);

    } else {
        sea_add_function_symbol(p->mod, proto);
    }
}

void parse_struct_expr(Parser *p, SeaFunctionGraph *fn, SeaType *t) {
    if (t) Assert(t->kind == SeaLatticeKind_Struct);
    Token tok = current_token(p);
    String8 struct_name = { 0 };
    if (tok.kind == TokenKind_Identifier) {
        struct_name = token_string(p, tok);
        advance_token(p);
        if (t && !str8_match(struct_name, t->s.name, 0)) {
            parser_error(p, "I fucking hate you chudd.");
        }
    }

    tok = current_token(p);
    if (tok.kind != TokenKind_LBrace) {
        parser_error(p, "I fucking hate you chudd.");
    }

}

SeaNode *parse_func_call_expr(Parser *p, SeaFunctionGraph *fn) {
    // enters in on the name token
    Token func_token = current_token(p);
    String8 func_name = token_string(p, func_token);

    SeaSymbolEntry *sym = sea_lookup_symbol(p->mod, func_name);
    SeaType *t = 0;
    SeaFunctionProto proto = { 0 };
    if (!sym) {
        parser_error(p, "%.*s is not defined", str8_varg(func_name));
    } else {
        if (sym->type->kind != SeaLatticeKind_Func) {
            parser_error(p, "%.*s is not defined as a function", str8_varg(func_name));
        } else {
            proto = sym->type->func;
            t = sym->type;
        }
    }

    advance_token(p);
    skip_newlines(p);
    Token tok = current_token(p);

    Assert(tok.kind == TokenKind_LParen); // how else would you guess;
    advance_token(p);
    skip_newlines(p);
    tok = current_token(p);

    U64 arg_count = 0;
    while (p->curr < p->tok_count && tok.kind != TokenKind_RParen) {
        SeaNode *expr = parse_expr(p, fn);
        // TODO(isaac): Type check

        arg_count += 1;
        skip_newlines(p);
        tok = current_token(p);
        if (tok.kind == TokenKind_Comma) {
            advance_token(p);
            skip_newlines(p);
        } else if (tok.kind == TokenKind_RParen) {
          break;
        } else {
            String8 str = token_string(p, tok);
            parser_error(p, "Unexpected token %.*s", str8_varg(str));
        }
    }

    if (arg_count != proto.args.count) {
        parser_error(p, "Expect %d args in %.*s got %d", (int)proto.args.count, str8_varg(func_name), arg_count);
    }

    advance_token(p);
}



void parse_decl(Parser *p) {


    Token tok = current_token(p);
    switch (tok.kind) {
        case TokenKind_Fn: {
            parse_func(p);
        } break;
        case TokenKind_Struct: {
            parse_struct(p);
        } break;
        default: {
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
