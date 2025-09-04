#include "parser.h"

TreeToken tree_current_token(TreeParser *p) {
    return p->tokens[p->curr];
}


void tree_debug_print_token(TreeParser *p, TreeToken tok) {
    String str = string_from_source(p->src, tok.start, tok.end);
    printf("'%.*s' = %d\n", (int)str.len, str.str, tok.kind);
}


TreeToken tree_peek_next(TreeParser *p) {
    if (p->curr + 1 < p->tokencount) {
        return p->tokens[p->curr + 1];
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

    printf("Urinary: "); tree_debug_print_token(p, tok);

    tree_advance_token(p);


    switch (tok.kind) {
        case TreeTokenKind_Int_Lit: {
            String intstr = string_from_source(p->src, tok.start, tok.end);
            S64 v = string_parse_int(intstr);
            return soup_create_const_int(&p->fn, v);
        }
        default: {
            String tok_str = string_from_source(p->src, tok.start, tok.end);
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
            SoupNode *ret = soup_create_return(&p->fn, expr);
            return ret;
        }
        default:
            // emit error
            break;
    }

    return NULL;
}
