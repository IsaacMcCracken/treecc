#include "sea_internal.h"

typedef *SeaNode(*SeaMachInstrSelectFn)(*SeaNode);
typedef *SeaNode(*SeaMachJumpFn)(void);
typedef *SeaNode(*SeaMachSplitFn)(void);

typedef struct SeaMach SeaMach;
struct SeaMach {
    String8 name;
    SeaMachInstrSelectFn select;
    SeaMachJumpFn jump;
    SeaMachSplitFn split;
}
