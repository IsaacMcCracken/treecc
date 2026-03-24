#include "sea_internal.h"


static inline RegMask rmask_u64(U64 mask) {
    RegMask rm = {0};
    rm.m[0] = mask;
    return rm;
}

static inline RegMask rmask_empty(void) {
    RegMask rm = {0};
    return rm;
}

static inline RegMask rmask_full(void) {
    RegMask rm;
    rm.m[0] = ~(U64)0;
    rm.m[1] = ~(U64)0;
    rm.m[2] = ~(U64)0;
    rm.m[3] = ~(U64)0;
    return rm;
}

static inline B32 rmask_get(RegMask rm, U8 reg) {
    return (rm.m[reg >> 6] >> (reg & 63)) & 1;
}

static inline RegMask rmask_set(RegMask rm, U8 reg) {
    rm.m[reg >> 6] |= (U64)1 << (reg & 63);
    return rm;
}

static inline RegMask rmask_unset(RegMask rm, U8 reg) {
    rm.m[reg >> 6] &= ~((U64)1 << (reg & 63));
    return rm;
}

static inline RegMask rmask_and(RegMask a, RegMask b) {
    RegMask rm;
    rm.m[0] = a.m[0] & b.m[0];
    rm.m[1] = a.m[1] & b.m[1];
    rm.m[2] = a.m[2] & b.m[2];
    rm.m[3] = a.m[3] & b.m[3];
    return rm;
}

static inline RegMask rmask_or(RegMask a, RegMask b) {
    RegMask rm;
    rm.m[0] = a.m[0] | b.m[0];
    rm.m[1] = a.m[1] | b.m[1];
    rm.m[2] = a.m[2] | b.m[2];
    rm.m[3] = a.m[3] | b.m[3];
    return rm;
}

static inline RegMask rmask_not(RegMask a) {
    RegMask rm;
    rm.m[0] = ~a.m[0];
    rm.m[1] = ~a.m[1];
    rm.m[2] = ~a.m[2];
    rm.m[3] = ~a.m[3];
    return rm;
}

static inline B32 rmask_empty_p(RegMask rm) {
    return (rm.m[0] | rm.m[1] | rm.m[2] | rm.m[3]) == 0;
}

static inline B32 rmask_eq(RegMask a, RegMask b) {
    return a.m[0] == b.m[0]
        && a.m[1] == b.m[1]
        && a.m[2] == b.m[2]
        && a.m[3] == b.m[3];
}

static inline S32 rmask_get_first_empty(RegMask rm) {
    for EachIndex(i, 0x100) {
        if (rmask_get(rm, i)) return (S32)i;
    }

    return (S32)(-1);
}
