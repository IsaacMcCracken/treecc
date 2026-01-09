#ifndef TREE_FRONT_H
#define TREE_FRONT_H

#include <core.h>

typedef struct CompilationUnit CompilationUnit;
struct CompilationUnit {
    CompilationUnit *next; //
    CompilationUnit *prev; //
    String filename;

}

typedef struct {

} State;


#endif // TREE_FRONT_H
