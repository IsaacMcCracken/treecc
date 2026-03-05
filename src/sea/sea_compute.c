#include "sea_internal.h"


SeaType *compute_int_const(SeaFunctionGraph *fn, SeaNode *n) {
    Assert(n->kind == SeaNodeKind_ConstInt);
    return sea_type_const_int(fn, n->vint);
}

SeaType *compute_int_urnary_op(SeaFunctionGraph *fn, SeaNode *n) {
    SeaNodeKind op = n->kind;
    SeaType *a = n->inputs[1]->type;
    SeaType c = { .kind = SeaLatticeKind_Int };
    SeaTypeInt out = { 0 };
    SeaTypeInt aint = a->i;

     // Assume S64 for now to keep it simple stupid
     S64 type_min = min_S64;
     S64 type_max = max_S64;


     switch (op) {
         case SeaNodeKind_NegI: {
             // Negation: -[min, max] = [-max, -min]
             // Special case: -S64_MIN overflows to S64_MIN (can't represent +9223372036854775808)
             if (aint.min == type_min) {
                 out.min = type_min;
                 out.max = type_max;
             } else if (sea_type_is_const_int(a)) {
                 return sea_type_const_int(fn, !aint.min);
             } else {
                 out.max = -aint.min;
                 out.min = -aint.max;
             }
         } break;

         case SeaNodeKind_Not: {
             if (sea_type_is_const_int(a)) {
                 out.min = !aint.min;
                 out.max = out.min;
             }

             out.min = 0;
             out.max = 1;

         } break;
     }

     c.i = out;
     return sea_type_canonical(fn->m, &c);
 }

 SeaType *compute_proj(SeaFunctionGraph *fn, SeaNode *n) {
     SeaNode *input = n->inputs[0];
     Assert(input->type->kind == SeaLatticeKind_Tuple);
     Assert(n->vint < input->type->tup.count);
     return input->type->tup.elems[n->vint];
 }



 // TODO change all compute functions to compute_xx(fn, node) rather than type
 SeaType *compute_int_bin_op(SeaFunctionGraph *fn, SeaNode *n) {
     SeaNodeKind op = n->kind;
     // TODO move 0,1 to 1,2 because of scheduling
     SeaType *a = n->inputs[1]->type;
     SeaType *b = n->inputs[2]->type;

     // todo(isaac) consider if this is okay
     if (!a || !b) {
         switch (op) {
             case SeaNodeKind_EqualI:
             case SeaNodeKind_NotEqualI:
             case SeaNodeKind_LesserEqualI:
             case SeaNodeKind_LesserThanI:
             case SeaNodeKind_GreaterEqualI:
             case SeaNodeKind_GreaterThanI:
                return &sea_type_Bool;
            default: return &sea_type_S64;
         }
     }

     if (a->kind != SeaLatticeKind_Int || b->kind != SeaLatticeKind_Int) {
         return &sea_type_Bot;
     }


     SeaType c = { .kind = SeaLatticeKind_Int };
     SeaTypeInt out = { 0 };
     SeaTypeInt aint = a->i;
     SeaTypeInt bint = b->i;

     // Assume S64 for now to keep it simple stupid
     S64 type_min = min_S64;
     S64 type_max = max_S64;

     switch (op) {
         case SeaNodeKind_AddI: {
             // Check if there will be an overflow on max
             B32 overflow_max = (bint.max > 0 && aint.max > type_max - bint.max);
             // Check if there will be an underflow on min
             B32 overflow_min = (bint.min < 0 && aint.min < type_min - bint.min);

             if (overflow_max || overflow_min) {
                 out.max = type_max;
                 out.min = type_min;
             } else {
                 out.max = aint.max + bint.max;
                 out.min = aint.min + bint.min;
             }
         } break;

         case SeaNodeKind_SubI: {
             // a - b = a + (-b), so max = a.max - b.min, min = a.min - b.max
             B32 overflow_max = (bint.min < 0 && aint.max > type_max + bint.min);
             B32 overflow_min = (bint.max > 0 && aint.min < type_min + bint.max);

             if (overflow_max || overflow_min) {
                 out.max = type_max;
                 out.min = type_min;
             } else {
                 out.max = aint.max - bint.min;
                 out.min = aint.min - bint.max;
             }
         } break;

         case SeaNodeKind_MulI: {
             // Try all 4 corner combinations
             S64 corners[4] = {
                 aint.min * bint.min,
                 aint.min * bint.max,
                 aint.max * bint.min,
                 aint.max * bint.max
             };

             // Check if any corner overflowed
             B32 overflow = 0;
             overflow |= (bint.min != 0 && corners[0] / bint.min != aint.min);
             overflow |= (bint.max != 0 && corners[1] / bint.max != aint.min);
             overflow |= (bint.min != 0 && corners[2] / bint.min != aint.max);
             overflow |= (bint.max != 0 && corners[3] / bint.max != aint.max);

             if (overflow) {
                 out.min = type_min;
                 out.max = type_max;
             } else {
                 S64 min_result = corners[0];
                 S64 max_result = corners[0];
                 for (int i = 1; i < 4; i++) {
                     if (corners[i] < min_result) min_result = corners[i];
                     if (corners[i] > max_result) max_result = corners[i];
                 }
                 out.min = min_result;
                 out.max = max_result;
             }
         } break;

         case SeaNodeKind_DivI: {
             // Division by range containing zero is undefined
             if (bint.min <= 0 && bint.max >= 0) {
                 out.min = type_min;
                 out.max = type_max;
             } else {
                 // Try all 4 corners
                 S64 corners[4] = {
                     aint.min / bint.min,
                     aint.min / bint.max,
                     aint.max / bint.min,
                     aint.max / bint.max
                 };

                 S64 min_result = corners[0];
                 S64 max_result = corners[0];
                 for (int i = 1; i < 4; i++) {
                     if (corners[i] < min_result) min_result = corners[i];
                     if (corners[i] > max_result) max_result = corners[i];
                 }
                 out.min = min_result;
                 out.max = max_result;
             }
         } break;

         // Comparison operations return 0 or 1
         case SeaNodeKind_EqualI: {
             // Can only be equal if ranges overlap
             if (aint.max < bint.min || aint.min > bint.max) {
                 // Ranges don't overlap - always false
                 out.min = 0;
                 out.max = 0;
             } else if (aint.min == aint.max &&
                        bint.min == bint.max &&
                        aint.min == bint.min) {
                 // Both are constants and equal - always true
                 out.min = 1;
                 out.max = 1;
             } else {
                 // Unknown - could be 0 or 1
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_NotEqualI: {
             // Opposite of equal
             if (aint.max < bint.min || aint.min > bint.max) {
                 // Ranges don't overlap - always true
                 out.min = 1;
                 out.max = 1;
             } else if (aint.min == aint.max &&
                        bint.min == bint.max &&
                        aint.min == bint.min) {
                 // Both are constants and equal - always false
                 out.min = 0;
                 out.max = 0;
             } else {
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_GreaterThanI: {
             // a > b
             if (aint.min > bint.max) {
                 // a always greater - always true
                 out.min = 1;
                 out.max = 1;
             } else if (aint.max <= bint.min) {
                 // a never greater - always false
                 out.min = 0;
                 out.max = 0;
             } else {
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_GreaterEqualI: {
             // a >= b
             if (aint.min >= bint.max) {
                 out.min = 1;
                 out.max = 1;
             } else if (aint.max < bint.min) {
                 out.min = 0;
                 out.max = 0;
             } else {
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_LesserThanI: {
             // a < b
             if (aint.max < bint.min) {
                 out.min = 1;
                 out.max = 1;
             } else if (aint.min >= bint.max) {
                 out.min = 0;
                 out.max = 0;
             } else {
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_LesserEqualI: {
             // a <= b
             if (aint.max <= bint.min) {
                 out.min = 1;
                 out.max = 1;
             } else if (aint.min > bint.max) {
                 out.min = 0;
                 out.max = 0;
             } else {
                 out.min = 0;
                 out.max = 1;
             }
         } break;

         case SeaNodeKind_ModI: {
            if (sea_type_is_const_int(a) && sea_type_is_const_int(b)) {
                S64 aval = sea_type_const_int_val(a);
                S64 bval = sea_type_const_int_val(b);
                S64 val = aval % bval;
                out.min = val;
                out.max = val;
            } else if (sea_type_is_const_int(b)) {
                S64 bval = sea_type_const_int_val(b);
                out.min = Min(0, aint.min % bval);
                out.max = bval - 1;
            } else {
                // TODO this is probably not correct behaviour
                out.min = type_min;
                out.max = type_max;
            }
         } break;
     }

     c.i = out;
     return sea_type_canonical(fn->m, &c);
 }



 SeaType *compute_if(SeaFunctionGraph *fn, SeaNode *ifnode) {

     // test prev control
     if (ifnode->inputs[0]->type != &sea_type_CtrlLive) {
        return &sea_type_IfNeth;
     }

     SeaNode *cond = ifnode->inputs[1];
     if (sea_type_is_const_int(cond->type)) {
         S64 v = sea_type_const_int_val(cond->type);
         if (v) {
             return &sea_type_IfTrue;
         } else {
             return &sea_type_IfFalse;
         }
     }

     return &sea_type_IfBoth;
 }

 SeaType *compute_region(SeaFunctionGraph *fn, SeaNode *region) {
     SeaType *t = &sea_type_CtrlDead;
     for EachIndexFrom(i, 1, region->inputlen) {
        SeaNode *input = region->inputs[i];
        // TODO figure out if this is valid or not
        // if (input)
        t = sea_type_meet(fn, t, input->type);
     }
     return t;
 }

 SeaType *compute_loop(SeaFunctionGraph *fn, SeaNode *node) {
     if (node->inputs[node->inputlen-1] == 0) {
         return &sea_type_CtrlLive;
     }

     return compute_region(fn, node);
 }

 SeaType *compute_phi(SeaFunctionGraph *fn, SeaNode *n) {
     SeaNode *region = n->inputs[0];
     if (region->kind != SeaNodeKind_Region || region->kind != SeaNodeKind_Loop) {
         return &sea_type_Bot;
     }

     if (region->inputs[region->inputlen - 1] == 0) return &sea_type_Bot;

    SeaType *t = &sea_type_Top;
    for EachIndexFrom(i, 1, n->inputlen) {
        SeaNode *input = n->inputs[i];
        // TODO it is not always safe to peephole phis
        // move sea_peephole out of create_phi and
        // create_region. then peephole once done.
        // maybe add a garbage collector
        t = sea_type_meet(fn, t, input->type);
    }
     return t;
 }


SeaType *sea_compute_type(SeaFunctionGraph *fn, SeaNode *n) {
     switch (n->kind) {
         case SeaNodeKind_EqualI:
         case SeaNodeKind_NotEqualI:
         case SeaNodeKind_GreaterThanI:
         case SeaNodeKind_GreaterEqualI:
         case SeaNodeKind_LesserThanI:
         case SeaNodeKind_LesserEqualI:
         case SeaNodeKind_ModI:
         case SeaNodeKind_AddI:
         case SeaNodeKind_SubI:
         case SeaNodeKind_MulI:
         case SeaNodeKind_DivI: {
             return compute_int_bin_op(fn, n);
         }
         case SeaNodeKind_Not:
         case SeaNodeKind_NegI: {
             return compute_int_urnary_op(fn, n);
         }

         case SeaNodeKind_If: {
             return compute_if(fn, n);
         }

         case SeaNodeKind_ConstInt: {
             return compute_int_const(fn, n);
         }

         case SeaNodeKind_Proj: {
             return compute_proj(fn, n);
        }

        case SeaNodeKind_Phi: {
            return compute_phi(fn, n);
        }

        case SeaNodeKind_Loop: {
            return compute_loop(fn, n);
        }
        case SeaNodeKind_Region: {
            return compute_region(fn, n);
        }
        case SeaNodeKind_Return: {
            return &sea_type_CtrlLive;
        }
        case SeaNodeKind_Dead: {
            return &sea_type_CtrlDead;
        }
        case SeaNodeKind_Stop: {
            return &sea_type_Bot;
        }
        default: {
            fprintf(stderr, "Unknown Node Kind %d", (int)n->kind);
            Trap();
        }
     }
 }
