#define KEYWORD_COUNT 6
#define KEYWORD_MAP_SIZE 53

String keywords[64] = {0};
TokenKind keyword_map[53] = {0};


void tree_init_token_maps(void) {
    keywords[TokenKind_Fn] = (String){"fn", 2};
    keywords[TokenKind_Return] = (String){"return", 6};
    keywords[TokenKind_Int] = (String){"int", 3};
    keywords[TokenKind_If] = (String){"if", 2};
    keywords[TokenKind_Else] = (String){"else", 4};
    keywords[TokenKind_While] = (String){"while", 5};
    keyword_map[40] = TokenKind_Fn;
    keyword_map[2] = TokenKind_Return;
    keyword_map[52] = TokenKind_Int;
    keyword_map[5] = TokenKind_If;
    keyword_map[11] = TokenKind_Else;
    keyword_map[37] = TokenKind_While;
}
