#include "node.h"


typedef struct TreeBasicBlock TreeBasicBlock;
struct TreeBasicBlock {
    TreeNode *start;
    TreeNode *end;

    S32 depth;
    TreeBasicBlock *dom;
};
