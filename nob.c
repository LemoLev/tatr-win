#define NOB_IMPLEMENTATION
#include "./thirdparty/nob.h"

#define BUILD_FOLDER "build/"

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-Wall");
    cmd_append(&cmd, "-Wextra");
    cmd_append(&cmd, "-Wswitch-enum");
    cmd_append(&cmd, "-Wno-unused-function");
    cmd_append(&cmd, "-fsanitize=undefined,memory");
    cmd_append(&cmd, "-pedantic");
    cmd_append(&cmd, "-ggdb");
    cmd_append(&cmd, "-o", BUILD_FOLDER"tatr");
    cmd_append(&cmd, "tatr.c");
    if (!cmd_run(&cmd)) return 1;
    return 0;
}
