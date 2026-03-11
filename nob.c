#define NOB_IMPLEMENTATION
#include "./thirdparty/nob.h"

#define BUILD_FOLDER "build/"

bool test_ls_filter_negation_of_complex_expression_in_parens(Cmd *cmd, String_Builder *sb)
{
    nob_log(INFO, "Running %s...", __func__);

    const char *test_stdout_path = BUILD_FOLDER"test_stdout.txt";
    cmd_append(cmd, BUILD_FOLDER"tatr");
    cmd_append(cmd, "ls");
    cmd_append(cmd, "-df");
    cmd_append(cmd, "-f");
    cmd_append(cmd, "not (tagged or .bug and .test and .foo and .bar)");
    if (!cmd_run(cmd, .stdout_path = test_stdout_path)) return 1;
    sb->count = 0;
    if (!read_entire_file(test_stdout_path, sb)) return 1;

    String_View expected_stdout = sv_from_cstr(
        "OP_TAGGED\n"
        "OP_TAG bug\n"
        "OP_TAG test\n"
        "OP_AND\n"
        "OP_TAG foo\n"
        "OP_AND\n"
        "OP_TAG bar\n"
        "OP_AND\n"
        "OP_OR\n"
        "OP_NOT\n");
    String_View actual_stdout = sb_to_sv(*sb);
    if (!sv_eq(expected_stdout, actual_stdout)) {
        nob_log(ERROR, "UNEXPECTED STDOUT!!!");
        nob_log(ERROR, "EXPECTED:");
        fprintf(stderr, SV_Fmt"\n", SV_Arg(expected_stdout));
        nob_log(ERROR, "ACTUAL:");
        fprintf(stderr, SV_Fmt"\n", SV_Arg(actual_stdout));
        return false;
    }
    nob_log(INFO, "OK");
    return true;
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    String_Builder sb = {0};

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

    if (!test_ls_filter_negation_of_complex_expression_in_parens(&cmd, &sb)) return 1;

    return 0;
}
