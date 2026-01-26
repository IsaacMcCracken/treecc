#include "../src/base/base_inc.h"
#include "../src/os/os_inc.h"
#include "keyword.c"


internal void entry_point(CmdLine *cmd_line)  {
    gen_keywords();
}

#include "../src/base/base_inc.c"
#include "../src/os/os_inc.c"
