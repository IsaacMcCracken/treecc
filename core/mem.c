#include <core.h>

U64 mem_align_backward(U64 x, U64 align) {
    return x & ~(align -1);
}

U64 mem_align_forward(U64 x, U64 align) {
    return mem_align_backward(x + (align - 1), align);
}

