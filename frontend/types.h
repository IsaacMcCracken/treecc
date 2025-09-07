#ifndef TREE_TYPE_H
#define TREE_TYPE_H

#include <core.h>

typedef U32 TreeTypeKind;
enum {
  TreeTypeKind_Invalid,
  TreeTypeKind_Number,
  TreeTypeKind_Pointer,
  TreeTypeKind_Array,
  TreeTypeKind_Struct,
  TreeTypeKind_Function,
};

typedef struct TreeType TreeType;
struct TreeType {
  TreeTypeKind kind;
};

typedef U8 TreeNumberKind;
enum {
    TreeNumberKind_U8,
    TreeNumberKind_U16,
    TreeNumberKind_U32,
    TreeNumberKind_U64,
    TreeNumberKind_S8,
    TreeNumberKind_S16,
    TreeNumberKind_S32,
    TreeNumberKind_S64,
    TreeNumberKind_F32,
    TreeNumberKind_F64,
    TreeNumberKind_COUNT,
};

typedef U8 TreeNumberFlags;
enum {
    TreeNumberFlag_IsFloat  = (1<<0),
    TreeNumberFlag_IsSigned = (1<<1),
};

typedef struct TreeNumber TreeNumber;
struct TreeNumber {
    TreeTypeKind kind;
    U8 size : 4;
    TreeNumberFlags flags : 4;
};

typedef struct TreePointer TreePointer;
struct TreePointer {
  TreeTypeKind kind;
  TreeTypeKind *base;
};

typedef struct TreeField TreeField;
struct TreeField {
    String name;
    TreeType *type;
    TreeField *prev, *next;
};

typedef struct TreeFieldList TreeFieldList;
struct TreeFieldList {
    U64 count;
    TreeField *head, *tail;
};

typedef struct TreeStruct TreeStruct;
struct TreeStruct {
    TreeTypeKind kind;
    String name;
    TreeField *fields;
};

typedef struct TreeFunction TreeFunction;
struct TreeFunction {
    TreeTypeKind kind;
    TreeFieldList *params;
    TreeType *returntype;
};

typedef struct TreeTypeMapCell TreeTypeMapCell;
struct TreeTypeMapCell {
    TreeType *type;
    TreeTypeMapCell *next;
};

typedef struct TreeTypeMap TreeTypeMap;
struct TreeTypeMap {
    Arena *arena;
    TreeTypeMapCell **cells;
    U64 cap;
};


extern TreeNumber numberinfo[10];

U64 tree_type_hash(TreeType *type);
B32 tree_type_equal(TreeType *a, TreeType *b);
void tree_type_map_insert(TreeTypeMap *map, TreeType *type);
TreeType *tree_type_map_lookup(TreeTypeMap *map, TreeType *type);

#endif // TREE_TYPE_H
