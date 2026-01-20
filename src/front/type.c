#include <sea/sea.h>

typedef struct SeaIntType SeaIntType;
struct SeaIntType {
    SeaDataKind kind;
    U8 size : 4;
    U8 sign : 4;
};
