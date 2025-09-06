#define KEYWORD_COUNT 2
#define KEYWORD_MAP_SIZE 37

String keywords[] = {
    [TreeTokenKind_Return] = (String){"return", 6},
    [TreeTokenKind_Int] = (String){"int", 3},
};
TreeTokenKind keyword_map[37] = {
    [4] = TreeTokenKind_Return,
    [28] = TreeTokenKind_Int,
};
