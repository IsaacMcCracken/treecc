#include "sea_internal.h"


#define DISABLE_PEEPS 0

SeaNode *sea_idealize_int(SeaFunctionGraph *fn, SeaNode *node) {
    // constfold urnary expression
    if (node->kind == SeaNodeKind_NegateI && (node->inputs[1]->kind == SeaNodeKind_ConstInt)) {
        return sea_create_const_int(fn, -node->inputs[1]->vint);
    } else  if (node->kind == SeaNodeKind_Not && node->inputs[1]->kind == SeaNodeKind_ConstInt) {
        return sea_create_const_int(fn, !node->inputs[1]->vint);
    }

    SeaNode *lhs = node->inputs[1];
    SeaNode *rhs = node->inputs[2];
    // Constfold binary expression
    if (lhs->kind == SeaNodeKind_ConstInt && rhs->kind == SeaNodeKind_ConstInt) {
        S64 v = 0;
        switch (node->kind) {
            case SeaNodeKind_EqualI: {
                v = lhs->vint == rhs->vint;
            } break;
            case SeaNodeKind_NotEqualI: {
                v = lhs->vint != rhs->vint;
            } break;
            case SeaNodeKind_GreaterEqualI: {
                v = lhs->vint >= rhs->vint;
            } break;
            case SeaNodeKind_GreaterThanI: {
                v = lhs->vint > rhs->vint;
            } break;
            case SeaNodeKind_LesserEqualI: {
                v = lhs->vint <= rhs->vint;
            } break;
            case SeaNodeKind_LesserThanI: {
                v = lhs->vint < rhs->vint;
            } break;
            case SeaNodeKind_AddI: {
                v = lhs->vint + rhs->vint;
            } break;
            case SeaNodeKind_SubI: {
                v = lhs->vint - rhs->vint;
            } break;
            case SeaNodeKind_MulI: {
                v = lhs->vint * rhs->vint;
            } break;
            case SeaNodeKind_DivI: {
                v = lhs->vint / rhs->vint;
            } break;
        }
        // look in so see if we need to kill nodes
        return sea_create_const_int(fn, v);
    }

    if (node->kind == SeaNodeKind_AddI || node->kind == SeaNodeKind_MulI) {

        // if 2 * x -> x * 2  or 2 * (x * 4) -> (x * 4) * 2
        // swap sides because of communtivity
        if (lhs->kind == SeaNodeKind_ConstInt && rhs->kind != SeaNodeKind_ConstInt) {
            return sea_create_bin_op(fn, node->kind, rhs, lhs);
        }


        // (x * 4) * 2 -> x * (4 * 2)
        // constant propagation
        if (lhs->kind == node->kind && lhs->inputs[2]->kind == SeaNodeKind_ConstInt) {
            SeaNode *new_rhs = sea_create_bin_op(fn, node->kind, lhs->inputs[2], rhs);
            SeaNode *new_expr = sea_create_bin_op(fn, node->kind, lhs->inputs[1], new_rhs);
            return new_expr;
        }

    }


    if (node->kind == SeaNodeKind_AddI && rhs == lhs) {
        if (lhs == rhs) {
            SeaNode *two = sea_create_const_int(fn, 2);
            return sea_create_bin_op(fn, SeaNodeKind_MulI, lhs, two);
        }

        if (rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 0) return lhs;
    }

    if (node->kind == SeaNodeKind_SubI && lhs == rhs) {
        SeaNode *zero = sea_create_const_int(fn, 0);
        return zero;
    }

    if (node->kind == SeaNodeKind_MulI && rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 1) {
        return lhs;
    }

    if (node->kind == SeaNodeKind_DivI) {
        if (lhs == rhs) return sea_create_const_int(fn, 1);
        if (rhs->kind == SeaNodeKind_ConstInt && rhs->vint == 1) return lhs;
    }

    if (sea_type_is_const_int(node->type)) {
        return sea_create_const_int(fn, sea_type_const_int_val(node->type));
    }

    return node;
}


SeaNode *sea_idealize_phi(SeaFunctionGraph *fn, SeaNode *node) {
    /**
     * x = Phi(5, x) -> Phi(5, 5) -> 5
     */

    for EachIndexFrom(i, 1, node->inputlen) {
        if (node->inputs[i] == node) {
            U16 idx = 1 + ((i - 1) % (node->inputlen - 1));
            node->inputs[i] = node->inputs[idx];
        }
    }

    /*
     * Phi(x, x, x) becomes x
     */
    SeaNode *live = 0;
    SeaNode *region = node->inputs[0];
    for EachIndexFrom(i, 1, node->inputlen) {
        SeaNode *in = node->inputs[i];
        if (region->inputs[i]->type != &sea_type_CtrlDead && in != node) {
            if (live == 0 || live == in) {
                live = node->inputs[i];
            } else {
                live = 0;
                break;
            }
        }
   }

   if (live) return live;

   /*
    * Phi(Op(a, b), Op(p, q), Op(x, y))
    * where Op is a consistent binary operation
    * We tranform into
    * Op(Phi(a, p, x), Phi(b, q, y))
    */

    B32 is_same_op = 1;
    for EachIndexFrom(i, 2, node->inputlen) {
        if (node->inputs[1]->kind != node->inputs[i]->kind) {
            is_same_op = 0;
            break;
        }
    }



    if (is_same_op && sea_node_is_bin_op(node->inputs[1])) {
        SeaNodeKind op = node->inputs[1]->kind;
        Temp scratch = scratch_begin(0, 0);

        SeaNode **lhss = push_array(scratch.arena, SeaNode*, node->inputlen);
        SeaNode **rhss = push_array(scratch.arena, SeaNode*, node->inputlen);
        // New phis will have the old region
        lhss[0] = node->inputs[0];
        rhss[0] = node->inputs[0];

        for EachIndexFrom(i, 1, node->inputlen) {
            SeaNode *input = node->inputs[i];
            lhss[i] = input->inputs[1];
            rhss[i] = input->inputs[2];
        }

        SeaNode *lhs = sea_create_phi(fn, lhss, node->inputlen);
        SeaNode *rhs = sea_create_phi(fn, rhss, node->inputlen);
        scratch_end(scratch);

        return sea_create_bin_op(fn, op, lhs, rhs);
    }

    return node;
}

SeaNode *sea_idealize_return(SeaFunctionGraph *fn, SeaNode *node) {
    if (node->inputs[0]->type == &sea_type_CtrlDead) {
        return node->inputs[0];
    }

    return node;
}

SeaNode *sea_idealize_stop(SeaFunctionGraph *fn, SeaNode *node) {
    for EachIndex(i, node->inputlen) {
        SeaNode *input = node->inputs[i];
        if (input->type == &sea_type_CtrlDead) {
            sea_node_remove_input(fn, node, i);
            i -= 1;
        }
    }


    printf("[ ");
    for EachIndex(i, node->inputlen) {
        printf("%p, ", node->inputs[i]);
    }
    printf("]\n");

    return node;
}


SeaNode *sea_idealize_proj(SeaFunctionGraph *fn, SeaNode *node) {
    SeaNode *ctrl = node->inputs[0];

    switch (ctrl->kind) {
        case SeaNodeKind_If: {
            SeaNode *cond = ctrl->inputs[1];
            if (cond->kind == SeaNodeKind_ConstInt) {
                Assert(!((~1)&node->vint)); // is bool
                /**
                 * Example
                 * if (1) -> type[Dead, Live]
                 * if we are true branch then then we have type Live
                 * Check type of if->type.tup(!val) to see if the other
                 * branch is dead.
                 *
                 * If the other branch is dead we return the control before the if
                 */
                if (ctrl->type->tup.elems[!(node->vint)] == &sea_type_CtrlDead)
                    return ctrl->inputs[0];
            }
        } break;
    }


    return node;
}

SeaNode *sea_idealize_region(SeaFunctionGraph *fn, SeaNode *node) {
    if (node->inputs[node->inputlen - 1] == 0) return node; // inprogress what the fuck are we doing
    // Find dead input
    U16 path = 0;
    for EachIndexFrom(i, 1, node->inputlen) {
        SeaNode *input = node->inputs[i];
        if (input->type == &sea_type_CtrlDead) {
            path = i;
            break;
        }
    }

    // if there is a dead input
    if (path != 0) {
        SeaUser *u = node->users;
        while (u) {
            SeaUser *next = u->next;
            SeaNode *user = sea_user_val(u);
            if (user->kind == SeaNodeKind_Phi) {
                sea_node_remove_input(fn, user, path);
            }
            u = next;
        }
        sea_node_remove_input(fn, node, path);
        if (node->inputlen == 2) {
            u = node->users;
            sea_node_keep(fn, node);
            while (u) {
                SeaUser *next = u->next;
                SeaNode *phi = sea_user_val(u);
                if (phi->kind == SeaNodeKind_Phi) {
                    sea_node_subsume(fn, phi, phi->inputs[1]);
                }
                u = next;
            }
            sea_node_unkeep(fn, node);

            return node->inputs[1];
        }
    }

    return node;
}



SeaNode *sea_idealize(SeaFunctionGraph *fn, SeaNode *node) {
    SeaNode *new = node;
    switch (node->kind) {
        case SeaNodeKind_Not:
        case SeaNodeKind_EqualI:
        case SeaNodeKind_NotEqualI:
        case SeaNodeKind_GreaterThanI:
        case SeaNodeKind_GreaterEqualI:
        case SeaNodeKind_LesserThanI:
        case SeaNodeKind_LesserEqualI:
        case SeaNodeKind_NegateI:
        case SeaNodeKind_AddI:
        case SeaNodeKind_SubI:
        case SeaNodeKind_MulI:
        case SeaNodeKind_DivI: {
            new = sea_idealize_int(fn, node);
        } break;

        case SeaNodeKind_Proj: {
            new = sea_idealize_proj(fn, node);
        } break;

        case SeaNodeKind_Region:
        case SeaNodeKind_Loop: {
            new = sea_idealize_region(fn, node);
        } break;

        case SeaNodeKind_Phi: {
            new = sea_idealize_phi(fn, node);
        } break;

        case SeaNodeKind_Return: {
            new = sea_idealize_return(fn, node);
        } break;

        case SeaNodeKind_Stop: {
            new = sea_idealize_stop(fn, node);
        } break;
    }


    return new;
}

SeaNode *sea_peephole_opt(SeaFunctionGraph *fn, SeaNode *node) {


    // Todo remove constant behavior from sea_idealize_int
    // into just checking if the type is constant then
    // also maybe separtating sea_idealize_int into opts
    // per kind rather than all arithmetic
    SeaType *old = sea_compute_type(fn, node);
    node->type = old;

    // replace identical node with its canonical node
    if (node->gvn == 0) {
        SeaNode *single = sea_map_lookup(&fn->map, node);
        if (single) {
            // todo join types because types might be different
            // single->type = sea_type_join(fn, node->type, single->type);
            return sea_dead_code_elim(fn, node, single);
        } else {
            U32 gvn = sea_node_hash(SeaNode *new = sea_idealize(fn, node);


                SeaNode *result = sea_dead_code_elim(fn, node, new);node);
            node->gvn = gvn;
            sea_map_insert(&fn->map, node);
        }
    }

    SeaNode *ideal = sea_idealize(fn, node);
    return ideal;
}

SeaNode *sea_peephole(SeaFunctionGraph *fn, SeaNode *node) {
    Assert(node->kind != SeaNodeKind_Scope);
    node->type = sea_compute_type(fn, node);

    if (DISABLE_PEEPS) return node;


    SeaNode *result = sea_peephole_opt(fn, node);

    if (result != node) {
        result = sea_dead_code_elim(fn, node, result);
    }

    return result;
}
