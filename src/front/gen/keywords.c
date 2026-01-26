#define KEYWORD_COUNT 6
#define KEYWORD_MAP_SIZE 53

String8 keywords[64] = {0};
TokenKind keyword_map[53] = {0};


void init_token_maps(void) {
    keywords[TokenKind_Fn] = (String8){"fn", 2};
    keywords[TokenKind_Return] = (String8){"return", 6};
    keywords[TokenKind_Int] = (String8){"int", 3};
    keywords[TokenKind_If] = (String8){"if", 2};
    keywords[TokenKind_Else] = (String8){"else", 4};
    keywords[TokenKind_While] = (String8){"while", 5};
    keyword_map[40] = TokenKind_Fn;
    keyword_map[2] = TokenKind_Return;
    keyword_map[52] = TokenKind_Int;
    keyword_map[5] = TokenKind_If;
    keyword_map[11] = TokenKind_Else;
    keyword_map[37] = TokenKind_While;
}

