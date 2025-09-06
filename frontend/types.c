#include "types.h"

TreeNumber numberinfo[10] = {
  [TreeNumberKind_U8]   = (TreeNumber){.kind = TreeTypeKind_Number, .size = 0, .flags = 0},
  [TreeNumberKind_U16]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 1, .flags = 0},
  [TreeNumberKind_U32]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 2, .flags = 0},
  [TreeNumberKind_U64]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 3, .flags = 0},
  [TreeNumberKind_S8]   = (TreeNumber){.kind = TreeTypeKind_Number, .size = 0, .flags = TreeNumberFlag_IsSigned},
  [TreeNumberKind_S16]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 1, .flags = TreeNumberFlag_IsSigned},
  [TreeNumberKind_S32]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 2, .flags = TreeNumberFlag_IsSigned},
  [TreeNumberKind_S64]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 3, .flags = TreeNumberFlag_IsSigned},
  [TreeNumberKind_F32]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 2, . flags = TreeNumberFlag_IsFloat},
  [TreeNumberKind_F64]  = (TreeNumber){.kind = TreeTypeKind_Number, .size = 2, . flags = TreeNumberFlag_IsFloat},
};



U64 tree_type_hash_dbj2(Byte *bytes, U64 len) {
    U64 hash = 5382;
    for (U64 i = 0; i < len; i++) {
        Byte c = bytes[i];
        hash = ((hash<<5) + hash) + c;
    }

    return hash;
}

B32 tree_type_equal(TreeType *a, TreeType *b) {
    if (a->kind != b->kind) return 0;

    switch (a->kind) {
        case TreeTypeKind_Number: {
            TreeNumber *na = (TreeNumber*)a;
            TreeNumber *nb = (TreeNumber*)b;

            return na->size == nb->size && na->flags == nb->flags;
        } break;

        case TreeTypeKind_Pointer: {
            TreePointer *pa = (TreePointer*)a;
            TreePointer *pb = (TreePointer*)b;

            return pa->base == pb->base;
        } break;

        case TreeTypeKind_Function: {
            TreeFunction *fa = (TreeFunction*)a;
            TreeFunction *fb = (TreeFunction*)b;

            return fa->returntype == fb->returntype; // TODO check params as well
        } break;

        case TreeTypeKind_Struct: {
            TreeStruct *sa = (TreeStruct*)a;
            TreeStruct *sb = (TreeStruct*)b;
            return string_cmp(sa->name, sb->name);  // TODO(Isaac) check fields should be the same as params in tree
        } break;
    }

    return 0;
}

U64 tree_type_hash(TreeType *type) {
    switch (type->kind) {
        case TreeTypeKind_Number: {
            TreeNumber *num = (TreeNumber*)num;
            return num->size | (num->flags << 4);
        } break;

        case TreeTypeKind_Pointer: {
            TreePointer *p = (TreePointer*)type;
            U64 hash = tree_type_hash_dbj2((Byte *)(&p->base), sizeof(TreeType*));
            return hash;
        } break;

        case TreeTypeKind_Function: {
            return 0; //TODO
        } break;

        case TreeTypeKind_Struct: {
            return 0; //TODO
        } break;
    }

    return 0;
}

TreeType *tree_type_map_lookup(TreeTypeMap *map, TreeType *type) {
    U64 hash = tree_type_hash(type);
    U64 hashv = hash % map->cap;
    TreeTypeMapCell **slot = &map->cells[hashv];
    while (*slot) {
        if (tree_type_equal((*slot)->type, type)) return (*slot)->type;
        slot = &(*slot)->next;
    }

    return 0;
}

void tree_type_map_insert(TreeTypeMap *map, TreeType *type) {
    U64 hash = tree_type_hash(type);
    U64 hashv = hash % map->cap;
    TreeTypeMapCell **slot = &map->cells[hashv];
    while (*slot) {
        if (tree_type_equal((*slot)->type, type)) return;
        slot = &(*slot)->next;
    }

    TreeTypeMapCell *cell = arena_push(map->arena, TreeTypeMapCell);
    cell->type = type;
    *slot = cell;
}
