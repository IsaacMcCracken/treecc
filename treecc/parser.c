#include "parser.h"

TreeToken tree_current_token(TreeParser *p) {
    return p->tokens[p->curr];
}


void tree_debug_print_token(TreeParser *p, TreeToken tok) {
    String str = string_from_source(p->src, tok.start, tok.end);
    printf("'%.*s' = %s\n", (int)str.len, str.str, tree_token_kind_strings[tok.kind]);
}

// void tree_error(TreeParser *p, U32 tokenindex, String msg) {
//     fprintf("Error: %.*s\n", (int)msg.len, msg.str);
// }


TreeToken tree_peek_next(TreeParser *p) {
    if (p->curr + 1 < p->tokencount) {
        return p->tokens[p->curr + 1];
    }
    return (TreeToken){.kind = TreeTokenKind_EOF};
}

String tree_string_from_token(TreeParser *p, TreeToken tok) {
  return string_from_source(p->src, tok.start, tok.end);
};

TreeToken tree_peek_n(TreeParser *p, U32 n) {
    if (p->curr + n < p->tokencount) {
        return p->tokens[p->curr + n];
    }
    return (TreeToken){.kind = TreeTokenKind_EOF};
}

void tree_advance_token(TreeParser *p) {
    p->curr += 1;
}

TreeType *tree_parse_type_number(TreeParser *p) {
    TreeToken tok = tree_current_token(p);
    switch (tok.kind) {
        case TreeTokenKind_Int: {
            TreeType *t = (TreeType*)&numberinfo[TreeNumberKind_S32];
            return t;
        }
    }

    assert(0);
    return 0;
}

TreeType *tree_parse_type(TreeParser *p) {
    TreeType *t = 0;
    TreeToken tok = tree_current_token(p);

    switch (tok.kind) {
        case TreeTokenKind_Int:{
            t = tree_parse_type_number(p);
        } break;

        default: {
            // error
        } break;
    }
    tree_advance_token(p);
    return t;
}

S32 tree_operatator_precedence(TreeToken tok) {
    switch (tok.kind) {
        // TODO Logical || value = 3
        // TODO Logical && value = 4
        // TODO Bitwise | value = 5
        // TODO Bitwise ^ value = 6
        // TODO Bitwise & value = 7
        case TreeTokenKind_LogicEqual:
        case TreeTokenKind_LogicNotEqual:
            return 8;
        case TreeTokenKind_LogicGreaterThan:
        case TreeTokenKind_LogicGreaterEqual:
        case TreeTokenKind_LogicLesserThan:
        case TreeTokenKind_LogicLesserEqual:
            return 9;
        // TODO Bit shit (value = 10)
        case TreeTokenKind_Minus:
        case TreeTokenKind_Plus:
            return 11;
        case TreeTokenKind_Star:
        case TreeTokenKind_Slash:
            return 13;
        default:
            return -1;
    }
}

TreeNode *tree_parse_urnary(TreeParser *p) {
    TreeToken tok = tree_current_token(p);


    tree_advance_token(p);


    switch (tok.kind) {
        case TreeTokenKind_Int_Lit: {
            String intstr = string_from_source(p->src, tok.start, tok.end);
            S64 v = string_parse_int(intstr);
            return tree_create_const_int(&p->fn, v);
        }
        case TreeTokenKind_Identifier: {
            String name = tree_string_from_token(p, tok);
            TreeNode *node = tree_scope_lookup_symbol(p->current_scope, name);
            return node;
        } break;

        case TreeTokenKind_Minus: {
            tree_advance_token(p);
            TreeNode *input = tree_parse_urnary(p);
            return tree_create_urnary_expr(&p->fn, TreeNodeKind_NegateI, input);
        } break;

        case TreeTokenKind_LogicNot: {
            tree_advance_token(p);
            TreeNode *input = tree_parse_urnary(p);
            return tree_create_urnary_expr(&p->fn, TreeNodeKind_Not, input);
        } break;

        // could be type conversion
        case TreeTokenKind_LParen: {
            tree_advance_token(p);
            tok = tree_current_token(p); // todo type conversion

            TreeNode *expr = tree_parse_expr(p);

            tok = tree_current_token(p);

            if (tok.kind != TreeTokenKind_RParen) {
                // emit error
            }

            tree_advance_token(p);
        } break;

        default: {
            String tok_str = string_from_source(p->src, tok.start, tok.end);
            //todo emit error
            fprintf(stderr, "Error: gd expect token '%.*s'\n", (int)tok_str.len, tok_str.str);
            assert(0);
        } break;
    }

    return 0;
}


TreeNode *tree_parse_binary_expr(TreeParser *p, TreeNode *lhs, S32 precedence) {
    TreeToken lookahead = tree_current_token(p);
    while (tree_operatator_precedence(lookahead) >= precedence) {
        TreeToken op = lookahead;
        tree_advance_token(p);

        TreeNode *rhs = tree_parse_urnary(p);
        lookahead = tree_current_token(p);
        while (tree_operatator_precedence(lookahead) > tree_operatator_precedence(op)) {
            rhs = tree_parse_binary_expr(p, rhs, tree_operatator_precedence(op) + 1);
            lookahead = tree_peek_next(p);
        }

        TreeNodeKind opkind = 0;
        switch (op.kind) {
            case TreeTokenKind_Plus:    opkind = TreeNodeKind_AddI; break;
            case TreeTokenKind_Minus:   opkind = TreeNodeKind_SubI; break;
            case TreeTokenKind_Star:    opkind = TreeNodeKind_MulI; break;
            case TreeTokenKind_Slash:   opkind = TreeNodeKind_DivI; break;
            case TreeTokenKind_LogicEqual: opkind  = TreeNodeKind_EqualI; break;
            case TreeTokenKind_LogicNotEqual: opkind  = TreeNodeKind_NotEqualI; break;
            case TreeTokenKind_LogicGreaterThan: opkind  = TreeNodeKind_GreaterThanI; break;
            case TreeTokenKind_LogicGreaterEqual: opkind  = TreeNodeKind_GreaterEqualI; break;
            case TreeTokenKind_LogicLesserThan: opkind  = TreeNodeKind_LesserThanI; break;
            case TreeTokenKind_LogicLesserEqual: opkind  = TreeNodeKind_LesserEqualI; break;
            default:
                //emit error
                fprintf(stderr, "Error: expected a binary operator.\n");
                break;
        }

        TreeNode *opnode = tree_create_binary_expr(&p->fn, opkind, lhs, rhs);
        lhs = opnode;
    }

    return lhs;
}

TreeNode *tree_parse_expr(TreeParser *p) {
    TreeNode *lhs = tree_parse_urnary(p);
    return tree_parse_binary_expr(p, lhs, 0);
}

TreeNode *tree_parse_local_decl_stmt(TreeParser *p) {
    TreeType *t = tree_parse_type(p);

    TreeToken tok = tree_current_token(p);

    if (tok.kind != TreeTokenKind_Identifier) {
        // error
    }
    String symbol_name = tree_string_from_token(p, tok);

    // TODO: put in the c symbol table

    tree_advance_token(p);
    tok = tree_current_token(p);

    switch (tok.kind) {
        case TreeTokenKind_SemiColon: break;
        case TreeTokenKind_Equals: {
            tree_advance_token(p);
            // TODO convert CType to TreeDataKind;
            TreeNode *expr = tree_parse_expr(p);
            tree_scope_insert_symbol(&p->scopes, p->current_scope, symbol_name, expr);

            return expr;
        } break;
        default: {
            // error

        } break;
    }

    return 0;
}


TreeNode *tree_parse_stmt(TreeParser *p, TreeNode *prev_ctrl) {
    TreeToken tok = tree_current_token(p);

    TreeNode *result = 0;

    switch (tok.kind) {
        // parse_return
        case TreeTokenKind_Return: {
            tree_advance_token(p);
            TreeNode *expr = tree_parse_expr(p);
            TreeNode *ret = tree_create_return(&p->fn, prev_ctrl, expr);
            // TODO: remove this
            p->ret = ret;
            result = ret;
        } break;
        case TreeTokenKind_Int: {
            TreeNode *expr = tree_parse_local_decl_stmt(p);
            result = expr;
        } break;
        // parse_if
        case TreeTokenKind_If: {
            tree_advance_token(p);
            tok = tree_current_token(p);
            if (tok.kind != TreeTokenKind_LParen) {
                // emit error
            }

            tree_advance_token(p);
            TreeNode *cond = tree_parse_expr(p);
            tree_node_print_expr_debug(cond);

            tok = tree_current_token(p);

            if (tok.kind == TreeTokenKind_RParen) {
                // emit error
            }

            tree_advance_token(p);
            tok = tree_current_token(p);

            TreeScopeTable *true_scope = p->current_scope;
            TreeScopeTable *false_scope = tree_duplicate_scope(&p->scopes, true_scope);

            TreeNode *if_node = tree_create_if(&p->fn, prev_ctrl);
            TreeNode *true_node = tree_create_proj(&p->fn, if_node, 0);
            TreeNode *false_node = tree_create_proj(&p->fn, if_node, 1);

            if (tok.kind == TreeTokenKind_LBrace) {
                tree_parse_scope(p, prev_ctrl);
            } else {
                TreeNode *stmt = tree_parse_stmt(p, prev_ctrl);
            }

            p->current_scope = false_scope;

            tok = tree_current_token(p);
            if (tok.kind == TreeTokenKind_Else) {
                tree_advance_token(p);
                tok = tree_current_token(p);
                if (tok.kind == TreeTokenKind_LBrace) {

                }
            }

            TreeNode *region = tree_create_region_for_if(&p->fn, true_node, false_node, 4);

            p->current_scope = tree_merge_scopes(&p->fn, region, true_scope, false_scope);
            tree_free_all_scopes(&p->scopes, false_scope);

            return region;
        } break;

        case TreeTokenKind_While: {
            tree_advance_token(p);
            tok = tree_current_token(p);
            if (tok.kind != TreeTokenKind_LParen) {
                // emit error
            }

            tree_advance_token(p);
            TreeNode *cond = tree_parse_expr(p);
            tree_node_print_expr_debug(cond);

            tok = tree_current_token(p);

            if (tok.kind == TreeTokenKind_RParen) {
                // emit error
            }

            tree_advance_token(p);
            tok = tree_current_token(p);

        } break;

        case TreeTokenKind_Identifier: {
            tree_advance_token(p);
            TreeToken lookahead = tree_current_token(p);
            switch (lookahead.kind) {
                // update stmt i.e. " x = x + 1; "
                case TreeTokenKind_Equals: {
                    String name = tree_string_from_token(p, tok);
                    tree_advance_token(p);
                    TreeNode *expr = tree_parse_expr(p);
                    tree_scope_update_symbol(p->current_scope, name, expr);
                }break;

                // void function call i.e. " fn(args); "
                case TreeTokenKind_LParen: {
                    // TODO
                } break;

                default: {
                    // emit error
                } break;
            }
        } break;
        default:
            // emit error
            break;
    }

    tok = tree_current_token(p);
    if (tok.kind != TreeTokenKind_SemiColon) {
        // emit error
        printf("miau: %d\n", tok.kind);
        printf("what the flip"); tree_debug_print_token(p, tok);
    }
    tree_advance_token(p);


    return result;
}


TreeFunction tree_parse_function_proto(TreeParser *p, TreeType *returntype) {
    // enters on a TreeTokenKind_LParen
    tree_advance_token(p);
    TreeToken tok = tree_current_token(p);

    TreeFieldList l = {0};


    U64 arg_count = 0;
    while (tok.kind != TreeTokenKind_RParen) {
        // Should enter on a type token,
        TreeType *t = tree_parse_type(p);
        tok = tree_current_token(p);
        if (tok.kind != TreeTokenKind_Identifier) {
            // error
        }

        String name = tree_string_from_token(p, tok);
        TreeNode *node = tree_create_proj(&p->fn, p->fn.start, arg_count);
        tree_scope_insert_symbol(&p->scopes, p->current_scope, name, node);

        TreeField *field = arena_push(p->arena, TreeField);
        field->type = t;
        field->name = name;

        tree_push_field(&l, field);

        tree_advance_token(p);
        tok = tree_current_token(p);

        if (tok.kind == TreeTokenKind_Comma){
            tree_advance_token(p);
            tok = tree_current_token(p);
            continue;
        } else if (tok.kind != TreeTokenKind_RParen) {
            // emit error
        }

        arg_count += 1;
    }

    tree_advance_token(p);

    TreeFunction signature = (TreeFunction){
        .kind = TreeTypeKind_Function,
        .params = l,
        .returntype = returntype,
    };

    return signature; // todo register function signature in typesystem
}

void tree_parse_scope(TreeParser *p, TreeNode *prev_ctrl) {
    // should enter on a '{' token
    tree_advance_token(p);

    // set up lower new scope
    TreeScopeTable *new_scope = tree_alloc_scope(&p->scopes, p->current_scope);
    p->current_scope = new_scope;


    TreeToken tok = tree_current_token(p);
    while (p->curr < p->tokencount && tok.kind != TreeTokenKind_RBrace) {
        tree_parse_stmt(p, prev_ctrl);
        tok = tree_current_token(p);
        // tree_debug_print_token(p, tok);
    }

    // pop the scope
    TreeScopeTable *prev = p->current_scope->prev;
    tree_free_single_scope(&p->scopes, p->current_scope);
    p->current_scope = prev;
}

TreeDecl *tree_parse_function_decl(TreeParser *p, String name, TreeType *returntype) {
    TreeFunction proto = tree_parse_function_proto(p, returntype);
    TreeToken tok = tree_current_token(p);

    // TODO CREATE START NODE THINGY
    p->fn.start = arena_push(p->fn.arena, TreeNode);
    p->fn.start->kind = TreeNodeKind_Start;


    if (tok.kind == TreeTokenKind_SemiColon) {
        // reigster in symbol table
    } else if (tok.kind != TreeTokenKind_LBrace) {
        // error
    }

    tree_parse_scope(p, p->fn.start);

    return 0;
}

TreeDecl *tree_parse_decl(TreeParser *p) {
    TreeType *t = tree_parse_type(p);
    TreeToken tok = tree_current_token(p);
    if (tok.kind != TreeTokenKind_Identifier) {
        // error
    }

    String name = tree_string_from_token(p, tok);

    tree_advance_token(p);
    tok = tree_current_token(p);
    switch (tok.kind) {
        // function definition or proto
        case TreeTokenKind_LParen: {
            return tree_parse_function_decl(p, name, t);
        } break;
        // initalized global
        case TreeTokenKind_Equals: {
        } break;

        // undefined global
        case TreeTokenKind_SemiColon: {
        } break;

        // error
        default: {
        } break;
    }

    return 0;
}
