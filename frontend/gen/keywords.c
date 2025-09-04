#define KEYWORD_COUNT 1
#define KEYWORD_MAP_SIZE 37

String keywords[] = {
    [TreeTokenKind_Return] = (String){"return", 6},
};
TreeTokenKind keyword_map[37] = {
    [4] = TreeTokenKind_Return,
};
