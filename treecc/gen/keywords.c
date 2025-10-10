#define KEYWORD_COUNT 5
#define KEYWORD_MAP_SIZE 53

String keywords[64] = {0};
TreeTokenKind keyword_map[53] = {0};


void tree_init_token_maps(void) {
    keywords[TreeTokenKind_Return] = (String){"return", 6};
    keywords[TreeTokenKind_Int] = (String){"int", 3};
    keywords[TreeTokenKind_If] = (String){"if", 2};
    keywords[TreeTokenKind_Else] = (String){"else", 4};
    keywords[TreeTokenKind_While] = (String){"while", 5};
    keyword_map[2] = TreeTokenKind_Return;
    keyword_map[52] = TreeTokenKind_Int;
    keyword_map[5] = TreeTokenKind_If;
    keyword_map[11] = TreeTokenKind_Else;
    keyword_map[37] = TreeTokenKind_While;
}

