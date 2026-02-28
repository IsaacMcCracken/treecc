#include <stdio.h>

#include <base/base_inc.h>
#include <os/os_inc.h>
#include <front/tokenizer.h>
#include <front/front.h>


internal void entry_point(CmdLine *cmd_line) {
    frontend_init();

    Module m = {.arena = arena_alloc(), .m = sea_create_module()};
    for EachNode(cmd, String8Node, cmd_line->inputs.first) {
            if (os_folder_path_exists(cmd->string)) {
                m.path = cmd->string;

                Temp scratch = scratch_begin(0, 0);
                OS_FileIter *iter = os_file_iter_begin(
                    scratch.arena,
                    cmd->string,
                    OS_FileIterFlag_SkipFolders|OS_FileIterFlag_SkipHiddenFiles
                );

                for (OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, iter, &info);) {
                    if (str8_ends_with(info.name, str8_lit(".c4"), 0)) {
                        module_add_file_and_parse(&m, info.name);
                    }
                }

                scratch_end(scratch);
            } else {
                printf("Error: Could not find module '%.*s'.\n", str8_varg(cmd->string));
                break;
            }
    }

    for EachNode(fn_node, SeaFunctionGraphNode, m.m.functions.first) {
        char buf[512];
        snprintf(buf, 512, "graphs/%.*s.dot", str8_varg(fn_node->fn.proto.name));
        sea_graphviz(buf, &fn_node->fn);
    }


    frontend_deinit();
}
