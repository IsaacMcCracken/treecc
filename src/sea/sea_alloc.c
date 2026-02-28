#include "sea_internal.h"

// SeaAllocator defined in sea_internal.h

U64 sea_alloc_size(U64 size) {
    Assert(size <= 4096);
    if (size <= 64) return AlignPow2(size, 8);
    else return u64_up_to_pow2(size);
}

void *sea_alloc(SeaFunctionGraph *fn, U64 size) {
    return arena_push(fn->alloc->arena, size, 8, 1);
    // Assert(size <= 4096);
    // if (size <= 64) {
    //     U64 new_size = AlignPow2(size, 8);
    //     U64 idx = (new_size>>3)-1;
    //     FreeNode *bucket = fn->alloc->small_buckets[idx];
    //     if (bucket) {
    //         fn->alloc->small_buckets[idx] = bucket->next;
    //         MemoryZero(bucket, new_size);
    //         return (void*)bucket;
    //     } else {
    //         return arena_push(fn->alloc->arena,  new_size, 8, 1);
    //     }
    // } else {
    //     U64 new_size = u64_up_to_pow2(size);
    //     U64 idx = u64_log2_floor(new_size) - 7;
    //     FreeNode *bucket = fn->alloc->huge_buckets[idx];
    //     if (bucket) {
    //         fn->alloc->huge_buckets[idx] = bucket->next;
    //         MemoryZero(bucket, new_size);
    //         return (void*)bucket;
    //     } else {
    //         return arena_push(fn->alloc->arena,  new_size, 8, 1);
    //     }
    // }
}


void sea_free(SeaFunctionGraph *fn, void *ptr, U64 size) {
    // Assert(size <= 4096);
    // if (size <= 64) {
    //     U64 new_size = AlignPow2(size, 8);
    //     U64 idx = (new_size>>3)-1;
    //     FreeNode *bucket = ptr;
    //     bucket->next = fn->alloc->small_buckets[idx];
    //     bucket->next = 0;
    //     fn->alloc->small_buckets[idx] = bucket;
    // } else {
    //     U64 new_size = u64_up_to_pow2(size);
    //     U64 idx = u64_log2_floor(new_size) - 7;
    //     FreeNode *bucket = ptr;
    //     bucket->next = 0;
    //     bucket->next = fn->alloc->huge_buckets[idx];
    //     fn->alloc->huge_buckets[idx] = bucket;
    // }
}
