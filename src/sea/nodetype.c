#include "node.h"


internal SeaDataType primitives[] = {
    {.kind = SeaDataType_I32},
};


typedef U8 IntTag;
enum {
    IntTag_Signed,
    IntTag_Unsigned,
};


typedef struct LatticeInt {
    IntTag tag;
    union {
        U64 umax;
        S64 smax;
    };

    union {
        U64 umin;
        S64 smin;
    };
} LatticeInt;
