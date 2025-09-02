
#define CORE_IMPLEMENTATION
#include "../include/core.h"
#include <stdio.h>


char *keywords[] = {
    "return",
};

char *keyword_enums[] = {
  "TreeTokenKind_Return",
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


int main() {
    int mapsize = 37;

    FILE *file = fopen("frontend/gen/keywords.c", "w");

    printf("f: %p\n", file);

    fprintf(file, "#define KEYWORD_COUNT %d\n", (int)keyword_count());
    fprintf(file, "#define KEYWORD_MAP_SIZE %d\n\n", mapsize);


    fprintf(file, "char *keywords[] = {\n");
    for (int i = 0; i < keyword_count(); i++) {
        fprintf(file, "    \"%s\",\n", keywords[i]);
    }
    fprintf(file, "};\n");

    fprintf(file, "TreeTokenKind keyword_map[%d] = {\n",mapsize);
    for (int i = 0; i < keyword_count(); i++) {
        U32 hash = tree_hash_string(StrLit(keywords[i]));
        fprintf(file, "    [0x%08x] = %s,\n", hash%mapsize, keyword_enums[i]);
    }
    fprintf(file, "};\n");

    fclose(file);
}
