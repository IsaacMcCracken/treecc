#include <core.h>

char *string_to_cstring(Arena *arena, String s) {
    char *cstring = arena_push_array(arena, char, s.len + 1);
    mem_cpy(cstring, s.str, s.len); // arena automatically pushes 0s so null terminated
    return cstring;
}

S32 string_cmp(String a, String b) {
    U64 len = (a.len < b.len) ? a.len : b.len;
    for (U64 i = 0; i < len; i++) {
        if (a.str[i] > b.str[i]) return 1;
        else if (a.str[i] < b.str[i]) return -1;
    }

    if (a.len > b.len) return 1;
    else if (a.len < b.len) return -1;
    return 0;
}

/*
 * TODO bullet proof
 */
S64 string_parse_int(String a) {
    S64 x = 0;
    for (U32 i = 0; i < a.len; i++) {
        x = x * 10 + (a.str[i] - '0');
    }

    return x;
}