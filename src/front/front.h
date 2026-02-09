#ifndef TREE_FRONT_H
#define TREE_FRONT_H

// #include <base/base_inc.h>
#include <front/tokenizer.h>
#include <sea/sea.h>

typedef struct Parser Parser;
typedef struct FileNode FileNode;
typedef struct FileList FileList;
typedef struct Module Module;

struct Parser {
    SeaModule *m;
    Arena *arena;
    String8 filename;
    String8 src;
    Token *tokens;
    U32 tok_count;
    U32 curr;
    U32 line;
};

struct FileNode {
    FileNode *next;
    Parser p;
};

struct FileList {
    FileNode *first;
    FileNode *last;
    U64 count;
};

struct  Module {
    String8 path;
    Arena *arena;
    SeaModule m;
    FileList files;
};



int frontend_init(void);
void frontend_deinit();


void module_add_file_and_parse(Module *m, String8 filename);

#endif // TREE_FRONT_H
