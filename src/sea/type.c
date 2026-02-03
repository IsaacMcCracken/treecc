#include "sea.h"
#include "sea_internal.h"


U64 sea_type_size(SeaDataType *type) {
    switch (type->kind) {
        case SeaDataKind_I32:
        case SeaDataKind_I64: return sizeof(SeaIntType);
    }

    return 0;
}

U64 sea_hash_type(SeaFunctionGraph *fn, SeaDataType *type) {
    U8 *type_data = (U8*)type;
    U64 size = sea_type_size(type);
    return u64_hash_from_str8((String){type_data, size});
}


SeaDataType *sea_type_meet(SeaDataType *a, SeaDataType *b) {

}


void sea_init(void) {

}
