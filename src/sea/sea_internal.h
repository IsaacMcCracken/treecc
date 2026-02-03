#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H

#include <base/base_inc.h>

typedef struct SeaIntType SeaIntType;
struct SeaIntType {
    SeaDataKind kind;
    union {
        Rng1U64 u64;
        Rng1S64 s64;
    };
};

void sea_node_set_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input, U16 slot);
U16 sea_node_append_input(SeaFunctionGraph *fn, SeaNode *node, SeaNode *input);
SeaDataType *sea_type_meet(SeaDataType *a, SeaDataType *b);

#endif // SEA_INTERNAL_H
