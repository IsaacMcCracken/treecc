#ifndef BASE_BITARRAY_H
#define BASE_BITARRAY_H


typedef struct BitArray BitArray;
struct BitArray {
    U64 *bits;
    U64 len; //in bits
};


BitArray bits_alloc(Arena *arena, U64 n);
B32 bits_set(BitArray *bits, U64 idx, B32 v);
B32 bits_get(BitArray *bits, U64 idx, B32 v);
void bits_clear(BitArray *bits);



#endif // BASE_BITARRAY_H
