#include <core.h>

String cstring_to_string(char *str) {
    return (String){
        .str = str,
        .len = (U64)strlen(str),
    };
}

String string_alloc(Arena *arena, const char *str) {
    String string =
        (String){ .str = arena_push_array(arena, char, sizeof(str) - 1),
                  .len = sizeof(str) - 1 };
    mem_cpy(string.str, str, sizeof(str) - 1);
    return string;
}
String string_cpy(Arena *arena, String source) {
    String string = (String){ .str = arena_push_array(arena, char, source.len),
                              .len = source.len };
    mem_cpy(string.str, source.str, string.len);
    return string;
}
String string_concat(Arena *arena, int count, ...) {
    U64 len = 0;
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        String arg = va_arg(args, String);
        len += arg.len;
    }
    va_end(args);

    String string =
        (String){ .str = arena_push_array(arena, char, len), .len = len };

    U64 offset = 0;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        String arg = va_arg(args, String);
        mem_cpy(&(string.str[offset]), arg.str, arg.len);
        offset += arg.len;
    }

    va_end(args);
    return string;
}

char *string_to_cstring(Arena *arena, String s) {
    char *cstring = arena_push_array(arena, char, s.len + 1);
    mem_cpy(cstring, s.str,
            s.len); // arena automatically pushes 0s so null terminated
    return cstring;
}

S32 string_cmp(String a, String b) {
    U64 len = (a.len < b.len) ? a.len : b.len;
    for (U64 i = 0; i < len; i++) {
        if (a.str[i] > b.str[i])
            return 1;
        else if (a.str[i] < b.str[i])
            return -1;
    }

    if (a.len > b.len)
        return 1;
    else if (a.len < b.len)
        return -1;
    return 0;
}

/*
 * TODO support for binary, octal, and hexadecimal
 */
S64 string_parse_int(String a) {
    static char numerals[16] = "0123456789abcdef";

    U32 i = 0;
    S64 x = 0;
    S64 sign = 1;
    S64 base = 10;

    if (a.len == 0) return 0;

    if (a.str[0] == '-') {
        sign = -1;
        i = 1;
    }

    if (a.str[i] == '0' && ((i + 1) < a.len)) {
        switch (a.str[i+1]) {
            case 'b': base = 2; break;
            case 'o': base = 8; break;
            case 'x': base = 16; break;
            default:
                if (a.str[i + 1] < '0' && a.str[i + 1] > '9') return sign*x;
        }
    }

    for (; i < a.len; i++) {
        // check if we are parsing a number
        if (a.str[i] < '0' && a.str[i] > '9') return sign * x;
        x = x * base + (a.str[i] - '0');
    }


    return sign * x;
}
