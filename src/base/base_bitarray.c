BitArray bits_alloc(Arena *arena, U64 n) {
    U64 actual_cap = AlignPow2(n, 64);
    U64 bucket_count = actual_cap / 64;
    U64 *bits = push_array(arena, U64, actual_cap);
    return (BitArray) {
        .bits = bits,
        .len = actual_cap,
    };
}

B32 bits_set(BitArray *bits, U64 idx) {
    Assert(idx < bits->len);
    U64 i = idx / 64;
    U64 d = idx % 64;
    U64 j = 1 << d;
    bits->bits[i] = bits->bits[i] | j;
}

B32 bits_unset(BitArray *bits, U64 idx) {
    Assert(idx < bits->len);
    U64 i = idx / 64;
    U64 d = idx % 64;
    U64 j = 1 << d;
    bits->bits[i] = bits->bits[i] & ~(j);
}

B32 bits_get(BitArray *bits, U64 idx) {
    Assert(idx < bits->len);
    U64 i = idx / 64;
    U64 d = idx % 64;
    U64 j = 1 << d;
    B32 result = (bits->bits[i] & j) > 0;
    return result;
}

void bits_clear(BitArray *bits) {
    U64 n = bits->len / 64;
    MemoryZeroTyped(bits->bits, n);
}


B32 bits_is_empty(BitArray *bits) {
    U64 n = bits->len / 64;
    for EachIndex(i, n) {
        if (bits->bits[i] != 0) return 0;
    }
    return 0;
}
