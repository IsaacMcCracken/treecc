#include "parser.h"

TreeToken tree_current_token(TreeParser *p) {
    return p->tokens[p->curr];
}


void tree_debug_print_token(TreeParser *p, TreeToken tok) {
    String str = string_from_source(p->src, tok.start, tok.end);
    printf("'%.*s' = %d\n", (int)str.len, str.str, tok.kind);
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

S32 tree_operatator_precedence(TreeToken tok) {
    switch (tok.kind) {
        case TreeTokenKind_Minus:
        case TreeTokenKind_Plus:
            return 10;
        case TreeTokenKind_Star:
        case TreeTokenKind_Slash:
            return 20;
        default:
            return -1;
    }
}

SoupNode *tree_parse_urinary(TreeParser *p) {
    TreeToken tok = tree_current_token(p);


    tree_advance_token(p);


    switch (tok.kind) {
        case TreeTokenKind_Int_Lit: {
            String intstr = string_from_source(p->src, tok.start, tok.end);
            S64 v = string_parse_int(intstr);
            return soup_create_const_int(&p->fn, v);
        }
        default: {
            String tok_str = string_from_source(p->src, tok.start, tok.end);
            //todo make error
            fprintf(stderr, "Error: Did not expect token '%.*s'\n", (int)tok_str.len, tok_str.str);
        } break;
    }

    return 0;
}


SoupNode *tree_parse_binary_expr(TreeParser *p, SoupNode *lhs, S32 precedence) {
    TreeToken lookahead = tree_current_token(p);
    // printf("lookahead: "); tree_debug_print_token(p, lookahead);
    while (tree_operatator_precedence(lookahead) >= precedence) {
        TreeToken op = lookahead;
        tree_advance_token(p);

        SoupNode *rhs = tree_parse_urinary(p);
        lookahead = tree_current_token(p);
        while (tree_operatator_precedence(lookahead) > tree_operatator_precedence(op)) {
            rhs = tree_parse_binary_expr(p, rhs, tree_operatator_precedence(op) + 1);
            lookahead = tree_peek_next(p);
        }

        SoupNodeKind opkind = 0;
        switch (op.kind) {
            case TreeTokenKind_Plus:    opkind = SoupNodeKind_AddI; break;
            case TreeTokenKind_Minus:   opkind = SoupNodeKind_SubI; break;
            case TreeTokenKind_Star:    opkind = SoupNodeKind_MulI; break;
            case TreeTokenKind_Slash:   opkind = SoupNodeKind_DivI; break;
            default:
                //emit error
                fprintf(stderr, "Error: expected a binary operator.\n");
                break;
        }

        SoupNode *opnode = soup_create_binary_expr(&p->fn, opkind, lhs, rhs);
        lhs = opnode;
    }

    return lhs;
}

SoupNode *tree_parse_expr(TreeParser *p) {
    SoupNode *lhs = tree_parse_urinary(p);
    return tree_parse_binary_expr(p, lhs, 0);
}




SoupNode *tree_parse_stmt(TreeParser *p) {
    TreeToken tok = tree_current_token(p);
    switch (tok.kind) {
        case TreeTokenKind_Return: {
            tree_advance_token(p);
            SoupNode *expr = tree_parse_expr(p);
            SoupNode *ret = soup_create_return(&p->fn, p->fn.start, expr);
            return ret;
        }
        default:
            // emit error
            break;
    }

    return NULL;
}

TreeType *tree_parse_type_number(TreeParser *p) {
    TreeToken tok = tree_current_token(p);
    switch (tok.kind) {
        case TreeTokenKind_Int:
            return (TreeType*)&numberinfo[TreeNumberKind_S32];
    }

    assert(0);
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

TreeFunction tree_parse_function_proto(TreeParser *p, TreeType *returntype) {
    // enters on a TreeTokenKind_LParen
    tree_advance_token(p);
    TreeToken tok = tree_current_token(p);

    TreeFieldList l = {0};

    while (tok.kind != TreeTokenKind_RParen) {
        // Should enter on a type token,
    printf("got here");
        TreeType *t = tree_parse_type(p);
        tok = tree_current_token(p);
        if (tok.kind != TreeTokenKind_Identifier) {
            // error
        }

        String name = tree_string_from_token(p, tok);

        TreeField *field = arena_push(p->arena, TreeField);
        field->type = t;
        field->name = name;
        printf("\nType = %p, %.*s\n", t, (int)name.len, name.str);

        tree_push_field(&l, field);

        tree_advance_token(p);
        tok = tree_current_token(p);

        if (tok.kind == TreeTokenKind_Comma){
            tree_advance_token(p);
            tok = tree_current_token(p);
            continue;
        } else if (tok.kind != TreeTokenKind_RParen) {
            // error
        }
    }

    TreeFunction signature = (TreeFunction){
        .kind = TreeTypeKind_Function,
        .params = l,
        .returntype = returntype,
    };

    return signature; // todo register function signature in typesystem
}

TreeDecl *tree_parse_decl(TreeParser *p) {
    TreeType *t = tree_parse_type(p);
    TreeToken tok = tree_current_token(p);
    if (tok.kind != TreeTokenKind_Identifier) {
        // error
    }

    String name = tree_string_from_token(p, tok);
    printf("\nType = %p, %.*s\n", t, (int)name.len, name.str);

    tree_advance_token(p);
    tok = tree_current_token(p);
    switch (tok.kind) {
        // function definition or proto
        case TreeTokenKind_LParen: {
            TreeFunction proto = tree_parse_function_proto(p, t);
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
