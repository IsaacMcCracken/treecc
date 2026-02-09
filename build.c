#define BUILD_ENTRY_DEFINING_UNIT 1

// C4 frontend
#include "src/bin/treebin_entry.c"
#include "src/front/front.c"
#include "src/front/parser.c"
#include "src/front/tokenizer.c"

// Standard library
#include "src/base/base_inc.c"
#include "src/os/os_inc.c"


// Sea Backend
#include "src/sea/node.c"
#include "src/sea/module.c"
#include "src/sea/scope.c"
#include "src/sea/type.c"
