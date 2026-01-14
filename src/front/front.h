#ifndef TREE_FRONT_H
#define TREE_FRONT_H

#include <core.h>
#include "tokenizer.h"

typedef struct SymbolMap SymbolMap;
struct SymbolMap {

};

typedef struct SrcFile SrcFile;
struct SrcFile {
    String name;
    String text;
    Token *tokens;
    U32 tok_count;
};

typedef struct SrcNode SrcNode;
struct SrcNode {
    SrcNode *prev;
    SrcFile *file;
    U32 curr;
};

typedef struct Parser Parser;
struct Parser {
    Arena *arena;
    SrcNode *src;
};

typedef struct SrcHashEntry SrcHashEntry;
struct SrcHashEntry {
    SrcHashEntry *next;
    SrcHashEntry *next_hash;
    SrcFile file;
};

typedef struct SrcFiles SrcFiles;
struct SrcFiles {
    Arena *arena;
    RWMutex lock;
    SrcHashEntry **lookup;
    U64 entry_cap;
    U64 file_count;
    SrcHashEntry *head;
    SrcHashEntry *tail;
};



void frontend_init(void);
void add_src_file(String filename);
SrcFiles *get_src_files(void);
#endif // TREE_FRONT_H
