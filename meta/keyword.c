
#define CORE_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>

char *keywords[] = {
    "fn",
    "return",
    "int",
    "if",
    "else",
    "while",
    "struct",
};

char *keyword_enums[] = {
    "TokenKind_Fn",
    "TokenKind_Return",
    "TokenKind_Int",
    "TokenKind_If",
    "TokenKind_Else",
    "TokenKind_While",
    "TokenKind_Struct",
};

U64 keyword_count() {
    return sizeof(keywords)/sizeof(char*);
}

U32 hash_keyword(String8 str) {
    U32 hash = 0;
    switch (str.size) {
        case 0: assert(0);
        case 1: case 2: case 3:
            for (U32 i = 0; i < str.size; i++) {
                hash = (hash << 8) | str.str[i];
            }
            return hash;
    }

    hash |= (U32)str.str[0] << 24;
    hash |= (U32)str.str[1] << 16;
    hash |= (U32)str.str[str.size - 2] << 8;
    hash |= (U32)str.str[str.size - 1];

    return hash;
}


void gen_keywords(void) {
    int mapsize = 53;

    FILE *file = fopen("src/front/gen/keywords.c", "w");

    printf("f: %p\n", file);

    fprintf(file, "#define KEYWORD_COUNT %d\n", (int)keyword_count());
    fprintf(file, "#define KEYWORD_MAP_SIZE %d\n\n", mapsize);


    fprintf(file, "String8 keywords[64] = {0};\n");
    fprintf(file, "TokenKind keyword_map[%d] = {0};\n\n\n", mapsize);

    fprintf(file, "void init_token_maps(void) {\n");
    for (int i = 0; i < keyword_count(); i++) {
        int len = strlen(keywords[i]);
        fprintf(file, "    keywords[%s] = (String8){\"%s\", %d};\n", keyword_enums[i], keywords[i], len);
    }

    for (int i = 0; i < keyword_count(); i++) {
        U32 hash = hash_keyword((String8){(U8*)keywords[i], strlen(keywords[i])});
        fprintf(file, "    keyword_map[%d] = %s;\n", hash%mapsize, keyword_enums[i]);
    }
    fprintf(file, "}\n\n");

    fclose(file);
}
