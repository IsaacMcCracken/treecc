
#define CORE_IMPLEMENTATION
#include "../include/core.h"
#include <stdio.h>


char *keywords[] = {
    "return",
    "int",
    "if",
    "else",
    "while",
<<<<<<< HEAD

=======
>>>>>>> 621d45624b515c3dd09fd370390546071c03803e
};

char *keyword_enums[] = {
  "TreeTokenKind_Return",
  "TreeTokenKind_Int",
  "TreeTokenKind_If",
  "TreeTokenKind_Else",
<<<<<<< HEAD
  "TreeTokenKind_While",
=======
  "TreeTokenKind_While"
>>>>>>> 621d45624b515c3dd09fd370390546071c03803e
};

U64 keyword_count() {
    return sizeof(keywords)/sizeof(char*);
}

U32 tree_hash_string(String str) {
    U32 hash = 0;
    switch (str.len) {
        case 0: assert(0);
        case 1: case 2: case 3:
            for (U32 i = 0; i < str.len; i++) {
                hash = (hash << 8) | str.str[i];
            }
            return hash;
    }

    hash |= (U32)str.str[0] << 24;
    hash |= (U32)str.str[1] << 16;
    hash |= (U32)str.str[str.len - 2] << 8;
    hash |= (U32)str.str[str.len - 1];

    return hash;
}


void gen_keywords(void) {
    int mapsize = 53;

    FILE *file = fopen("treecc/gen/keywords.c", "w");

    printf("f: %p\n", file);

    fprintf(file, "#define KEYWORD_COUNT %d\n", (int)keyword_count());
    fprintf(file, "#define KEYWORD_MAP_SIZE %d\n\n", mapsize);


    fprintf(file, "String keywords[64] = {0};\n");
    fprintf(file, "TreeTokenKind keyword_map[%d] = {0};\n\n\n", mapsize);

    fprintf(file, "void tree_init_token_maps(void) {\n");
    for (int i = 0; i < keyword_count(); i++) {
        int len = strlen(keywords[i]);
        fprintf(file, "    keywords[%s] = (String){\"%s\", %d};\n", keyword_enums[i], keywords[i], len);
    }

    for (int i = 0; i < keyword_count(); i++) {
        U32 hash = tree_hash_string((String){keywords[i], strlen(keywords[i])});
        fprintf(file, "    keyword_map[%d] = %s;\n", hash%mapsize, keyword_enums[i]);
    }
    fprintf(file, "}\n\n");

    fclose(file);
}
