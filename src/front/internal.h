#ifndef SEA_INTERNAL_H
#define SEA_INTERNAL_H


#include <sea/sea.h>

typedef struct SeaIntType SeaIntType;
struct SeaIntType {
    SeaDataKind kind;
    U8 size : 4;
    U8 sign : 4;
};


extern SeaIntType int_types = {
    {.kind = SeaDataType_I32, .size = 2, .sign = 1},
}
#endif // SEA_INTERNAL_H
