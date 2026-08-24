#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#include "./thirdparty/nob.h"
#define FLAG_IMPLEMENTATION
#include "./thirdparty/flag.h"

#define BUILD_FOLDER      "build/"
#define SRC_FOLDER        "src/"
#define THIRDPARTY_FOLDER "thirdparty/"

typedef struct {
    Cmd cmd;
    String_Builder sb_stdout;
    String_Builder sb_stderr;
    bool tatr_ls_ok;
} Test_Runner;

bool run_filter_payload(Test_Runner *r, const char *filter_payload)
{
    const char *test_stdout_path = BUILD_FOLDER"test_stdout.txt";
    const char *test_stderr_path = BUILD_FOLDER"test_stderr.txt";
    cmd_append(&r->cmd, BUILD_FOLDER"tatr");
    cmd_append(&r->cmd, "ls");
    cmd_append(&r->cmd, "-debug");
    cmd_append(&r->cmd, filter_payload);
    Log_Handler *saved_log_handler = get_log_handler();
    set_log_handler(null_log_handler);
    {
        r->tatr_ls_ok = cmd_run(
            &r->cmd,
            .stdout_path = test_stdout_path,
            .stderr_path = test_stderr_path
        );
    }
    set_log_handler(saved_log_handler);
    r->sb_stdout.count = 0;
    if (!read_entire_file(test_stdout_path, &r->sb_stdout)) return false;
    r->sb_stderr.count = 0;
    if (!read_entire_file(test_stderr_path, &r->sb_stderr)) return false;
    return true;
}

bool assert_test_output(const char *output_label, String_View expected_stdout, String_View actual_stdout)
{
    if (!sv_eq(expected_stdout, actual_stdout)) {
        nob_log(ERROR, "UNEXPECTED %s !!!", output_label);
        nob_log(ERROR, "EXPECTED:");
        fprintf(stderr, SV_Fmt"\n", SV_Arg(expected_stdout));
        nob_log(ERROR, "ACTUAL:");
        fprintf(stderr, SV_Fmt"\n", SV_Arg(actual_stdout));
        return false;
    }
    return true;
}

bool test_ls_filter_negation_of_complex_expression_in_parens(Test_Runner *r)
{
    nob_log(INFO, "Running %s...", __func__);
    if (!run_filter_payload(r, "not (tagged or .bug and .test and .foo and .bar)")) return 1;
    if (!r->tatr_ls_ok) {
        fprintf(stderr, "%s:%d: ERROR: Command failed, but should've suceeded\n", __FILE__, __LINE__);
        return false;
    }
    if (!assert_test_output(
        "STDOUT",
        sv_from_cstr(
            "OP_TAGGED\n"
            "OP_TAG bug\n"
            "OP_TAG test\n"
            "OP_AND\n"
            "OP_TAG foo\n"
            "OP_AND\n"
            "OP_TAG bar\n"
            "OP_AND\n"
            "OP_OR\n"
            "OP_NOT\n"),
        sb_to_sv(r->sb_stdout))) return false;
    if (!assert_test_output("STDERR", (String_View){0}, sb_to_sv(r->sb_stderr))) return false;
    nob_log(INFO, "OK");
    return true;
}

bool test_ls_filter_not_stuck_to_open_paren(Test_Runner *r)
{
    nob_log(INFO, "Running %s...", __func__);
    if (!run_filter_payload(r, "not(.bug and .test) and .filter")) return false;
    if (!r->tatr_ls_ok) {
        nob_log(ERROR, "Command failed, but should've suceeded");
        return false;
    }
    if (!assert_test_output(
        "STDOUT",
        sv_from_cstr(
            "OP_TAG bug\n"
            "OP_TAG test\n"
            "OP_AND\n"
            "OP_NOT\n"
            "OP_TAG filter\n"
            "OP_AND\n"),
        sb_to_sv(r->sb_stdout))) return false;
    if (!assert_test_output("STDERR", (String_View){0}, sb_to_sv(r->sb_stderr))) return false;
    nob_log(INFO, "OK");
    return true;
}

bool test_ls_filter_report_error_utf8(Test_Runner *r)
{
    nob_log(INFO, "Running %s...", __func__);
    if (!run_filter_payload(r, ".привет hello")) return false;
    if (r->tatr_ls_ok) {
        nob_log(ERROR, "Command succeeded, but should've failed");
        return false;
    }
    if (!assert_test_output("STDOUT", (String_View){0}, sb_to_sv(r->sb_stdout))) return false;
    if (!assert_test_output(
        "STDERR",
        sv_from_cstr(
            ".привет hello\n"
            "        ^\n"
            "ERROR: Expected keywords `and`, or `or`\n"),
        sb_to_sv(r->sb_stderr))) return false;
    nob_log(INFO, "OK");
    return true;
}

void cc(Cmd *cmd)
{
    cmd_append(cmd, "clang");
    cmd_append(cmd, "-Wall");
    cmd_append(cmd, "-Wextra");
    cmd_append(cmd, "-Wswitch-enum");
    cmd_append(cmd, "-Wno-unused-function");
    cmd_append(cmd, "-fsanitize=undefined,memory");
    cmd_append(cmd, "-I.");
    cmd_append(cmd, "-I"THIRDPARTY_FOLDER);
    cmd_append(cmd, "-I"BUILD_FOLDER);
    if (0) cmd_append(cmd, "-pedantic");
    cmd_append(cmd, "-ggdb");
}

const char *get_current_date_rfc822(void)
{
    static const char *WEEKDAYS[] = {
        "Mon", "Tue", "Wed",
        "Thu", "Fri", "Sat",
        "Sun",
    };

    static const char *MONTHS[] = {
        "Jan", "Feb", "Mar",
        "Apr", "May", "Jun",
        "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec",
    };

    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);

    String_Builder sb = {0};
    sb_appendf(&sb, "%s, ",  WEEKDAYS[timeinfo->tm_wday - 1]);
    sb_appendf(&sb, "%02d ", timeinfo->tm_mday);
    sb_appendf(&sb, "%s ",   MONTHS[timeinfo->tm_mon]);
    sb_appendf(&sb, "%04d ", timeinfo->tm_year+1900);
    sb_appendf(&sb, "%02d:", timeinfo->tm_hour);
    sb_appendf(&sb, "%02d:", timeinfo->tm_min);
    sb_appendf(&sb, "%02d ", timeinfo->tm_sec);
    const char *zone = timeinfo->tm_zone;
    assert(strlen(zone) == 3);
    sb_append_cstr(&sb, zone);
    sb_append_cstr(&sb, "00");
    sb_append_null(&sb);
    return sb.items;
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    Procs procs = {0};

    bool no_test = false;
    bool run = false;
    bool help = false;
    flag_bool_var(&no_test, "no-test", false, "Do not run tests after building");
    flag_bool_var(&run, "run", false, "Run the app after the build");
    flag_bool_var(&help, "help", false, "Print this help message");

    if (!flag_parse(argc, argv)) {
        fprintf(stderr, "Usage: %s [OPTIONS]\n", flag_program_name());
        flag_print_options(stderr);
        flag_print_error(stderr);
        return 1;
    }

    argc = flag_rest_argc();
    argv = flag_rest_argv();

    if (help) {
        fprintf(stderr, "Usage: %s [OPTIONS]\n", flag_program_name());
        flag_print_options(stderr);
        return 0;
    }

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

    cmd_append(&cmd, "git");
    cmd_append(&cmd, "rev-parse");
    cmd_append(&cmd, "HEAD");
    if (!cmd_run(&cmd, .stdout_path = BUILD_FOLDER"git_hash.txt")) return 1;

    String_Builder sb_git_hash = {0};
    sb_appendf(&sb_git_hash, "#ifndef GIT_HASH_H_\n");
    sb_appendf(&sb_git_hash, "#define GIT_HASH_H_\n");
    sb_appendf(&sb_git_hash, "#define GIT_HASH \"");
    if (!read_entire_file(BUILD_FOLDER"git_hash.txt", &sb_git_hash)) return 1;
    while (sb_git_hash.count > 0 && isspace(da_last(&sb_git_hash))) {
        da_pop(&sb_git_hash);
    }
    sb_appendf(&sb_git_hash, "\"\n");


    sb_appendf(&sb_git_hash, "#define BUILD_TIME \"%s\"\n", get_current_date_rfc822());
    sb_appendf(&sb_git_hash, "#endif // GIT_HASH_H_\n");
    if (!write_entire_file(BUILD_FOLDER"git_hash.h", sb_git_hash.items, sb_git_hash.count)) return 1;

    cc(&cmd);
    cmd_append(&cmd, "-o", BUILD_FOLDER"tatr");
    cmd_append(&cmd, SRC_FOLDER"tatr.c");
    if (!cmd_run(&cmd)) return 1;

    cc(&cmd);
    cmd_append(&cmd, "-DTASKS_TEST");
    cmd_append(&cmd, "-o", BUILD_FOLDER"tatr-test");
    cmd_append(&cmd, SRC_FOLDER"tatr.c");
    if (!cmd_run(&cmd)) return 1;

    if (!no_test) {
        cmd_append(&cmd, BUILD_FOLDER"tatr-test");
        if (!cmd_run(&cmd)) return 1;

        Test_Runner r = {0};
        if (!test_ls_filter_negation_of_complex_expression_in_parens(&r)) return 1;
        if (!test_ls_filter_not_stuck_to_open_paren(&r)) return 1;
        if (!test_ls_filter_report_error_utf8(&r)) return 1;
    }

    if (run) {
        cmd_append(&cmd, BUILD_FOLDER"tatr");
        da_append_many(&cmd, argv, argc);
        if (!cmd_run(&cmd)) return false;
    }

    return 0;
}
