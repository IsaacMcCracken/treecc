#ifndef CORE_INTERNAL_H
#define CORE_INTERNAL_H

#include <core.h>

typedef struct {
    Arena *arenas[2];
} ThreadContext;

#endif
