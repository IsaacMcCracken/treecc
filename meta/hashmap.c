#include <core.h>


gen_hashmap(
    const char *filename,
    const char *sprefix, 
    const char *fnprefix,
    const char *hashfn, 
    const char *equalfn,
    const char *inlinedata
) {
    File *file = fopen(filename, "w");

    fprintf(file, "typedef struct %sCell %sCell;\n", sprefix, sprefix)
    fprintf(file, "struct %sCell {\n")
    fprintf(file "    %sCell *next;\n", sprefix)
    fprintf(file "    %s;\n", inlinedata)
    fprintf(file, "};\n\n")

    fprintf(file, "typedef struct %sMap %sMap;\n", sprefix, sprefix)
    fprintf(file, "struct %sMap {\n")
    fprintf(file "    %sCell **cells;\n", sprefix)
    fprintf(file "    U64 cap;\n")
    fprintf(file, "};\n\n")

    

    fclose(file);
}