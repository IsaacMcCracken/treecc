#define KEYWORD_COUNT 2
#define KEYWORD_MAP_SIZE 53

String keywords[64];
TreeTokenKind keyword_map[53];


void tree_init_token_maps(void) {
    keywords[TreeTokenKind_Return] = (String){"return", 6};
    keywords[TreeTokenKind_Int] = (String){"int", 3};
    keyword_map[2] = TreeTokenKind_Return;
    keyword_map[52] = TreeTokenKind_Int;
}

