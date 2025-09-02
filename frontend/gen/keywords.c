#define KEYWORD_COUNT 1
#define KEYWORD_MAP_SIZE 37

char *keywords[] = {
    "return",
};
TreeTokenKind keyword_map[37] = {
    [0x00000011] = TreeTokenKind_Return,
};
