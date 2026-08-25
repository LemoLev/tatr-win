// Simple tool for manipulating tasks database
#include "nob.h"
#include "flag.h"
#include "ht.h"

#include "path.h"
#include "huid.h"
#include "md.h"
#include "git_hash.h"

#define DEFAULT_TASK_TITLE "New Task"
#define DEFAULT_PRIORITY 100

typedef struct {
    String_View *items;
    size_t count;
    size_t capacity;
} Tags;

bool tags_contains(Tags tags, String_View tag)
{
    for (size_t i = 0; i < tags.count; ++i) {
        if (sv_eq(tags.items[i], tag)) return true;
    }
    return false;
}

typedef struct {
    char *id;
    String_View title;
    String_View status;
    Tags tags;
    int priority;
    String_View task_md_content;
} Task;

static void append_task_md_content(String_Builder *sb, Task task)
{
    sb_appendf(sb, "# "SV_Fmt"\n", SV_Arg(task.title));
    sb_appendf(sb, "\n");
    sb_appendf(sb, "- STATUS: "SV_Fmt"\n", SV_Arg(task.status));
    sb_appendf(sb, "- PRIORITY: %d\n", task.priority);
    sb_appendf(sb, "- TAGS: ");
    for (size_t i = 0; i < task.tags.count; ++i) {
        if (i > 0) sb_appendf(sb, ",");
        sb_append_buf(sb, task.tags.items[i].data, task.tags.items[i].count);
    }
    sb_appendf(sb, "\n");
    sb_appendf(sb, "\n");
    sb_appendf(sb, "No description.\n");
}

// TASK(20260308-163429): Tasks array should be a hash table
typedef struct {
    Task *items;
    size_t count;
    size_t capacity;
} Tasks;

static void print_tags(Tags tags)
{
    for (size_t i = 0; i < tags.count; ++i) {
        if (i > 0) printf(",");
        printf(SV_Fmt, SV_Arg(tags.items[i]));
    }
}

static void print_task(const char *rel_path, Task *task)
{
    if (task->tags.count) {
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d, TAGS: ", rel_path, task->id, task->priority);
        print_tags(task->tags);
        printf("] "SV_Fmt"\n", SV_Arg(task->title));
    } else {
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d] "SV_Fmt"\n", rel_path, task->id, task->priority, SV_Arg(task->title));
    }
}

static bool get_current_dir_path(Path *cwd_path)
{
    const char *cwd_path_cstr = get_current_dir_temp();
    if (!cwd_path_cstr) return false;
    path_parse(cwd_path, sv_from_cstr(get_current_dir_temp()));
    return true;
}

static bool find_tasks_database(Path *dir_path)
{
    bool result = false;
    String_Builder sb_path = {0};

    if (!get_current_dir_path(dir_path)) return_defer(false);
    assert(dir_path->count > 0 && sv_eq(dir_path->items[0], SVLIT("")) && "CWD must be absolute");
    while (dir_path->count > 1) {
        da_append(dir_path, SVLIT("tasks"));
        const char *path = path_render_cstr(&sb_path, *dir_path);
        if (file_exists(path)) {
            File_Type type = get_file_type(path);
            if (type < 0) return_defer(false);
            if (type == FILE_DIRECTORY) return_defer(true);
        }
        dir_path->count -= 2;
    }
    nob_log(ERROR, "Could not find tasks/ folder");
    return_defer(false);
defer:
    free(sb_path.items);
    return result;
}

static bool load_tasks(Tasks *tasks, const char *dir_path)
{
    bool result = true;
    File_Paths children = {0};

    if (!read_entire_dir(dir_path, &children)) return_defer(false);
    size_t checkpoint = temp_save();
    for (size_t i = 0; i < children.count; ++i) {
        temp_rewind(checkpoint);
        const char *id = children.items[i];
        if (*id == '.') continue;
        if (!is_valid_huid(id)) continue;
        const char *task_path = temp_sprintf("%s/%s", dir_path, id);
        File_Type type = get_file_type(task_path);
        if (type < 0) return_defer(false);
        if (type != FILE_DIRECTORY) {
            nob_log(ERROR, "%s is not a directory", id);
            return_defer(false);
        }
        // This String_Builder becomes owned by Task.task_md_content.
        // So no memory deallocation is needed for it in here.
        String_Builder sb_content = {0};
        const char *task_md_path = temp_sprintf("%s/%s/TASK.md", dir_path, id);
        // TASK(20260308-171346): there should be a command that reports all the skipped weird folders and files found in the tasks/ folder
        if (!file_exists(task_md_path)) continue; // Ignore task folders without TASK.md
        if (!read_entire_file(task_md_path, &sb_content)) return_defer(false);
        sb_append_null(&sb_content);

        Md md = {0};
        md_init(&md, (char*)task_md_path, sb_content.items);

        String_View title = {0};
        if (!task_md_extract_title(&md, &title)) return_defer(false);
        String_View status = {0};
        if (!task_md_extract_field(&md, "STATUS", &status)) return_defer(false);
        String_View priority = {0};
        if (!task_md_extract_field(&md, "PRIORITY", &priority)) return_defer(false);
        Tags tags = {0};
        md_trim_spaces(&md);
        if (md_char(&md) == '-') {
            String_View sv = {0};
            if (!task_md_extract_field(&md, "TAGS", &sv)) return_defer(false);
            sv = sv_trim_left(sv);
            while (sv.count > 0) {
                String_View tag = sv_trim(sv_chop_by_delim(&sv, ','));
                da_append(&tags, tag);
                sv = sv_trim_left(sv);
            }
        }

        da_append(tasks, ((Task) {
            .id              = strdup(id),
            .title           = title,
            .status          = status,
            .priority        = atoi(temp_sv_to_cstr(priority)),
            .tags            = tags,
            .task_md_content = sb_to_sv(sb_content),
        }));
    }

defer:
    free(children.items);
    return result;
}

typedef int (*Task_Compare)(const void *a, const void *b);

static int task_compare_id(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return strcmp(ta->id, tb->id);
}

static int task_compare_id_reverse(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return strcmp(tb->id, ta->id);
}

static int task_compare_priority_reverse(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return tb->priority - ta->priority;
}

static int task_compare_priority(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return ta->priority - tb->priority;
}

static Task_Compare task_sorter(bool by_id, bool ascending)
{
    if (by_id) {
        if (ascending) {
            return task_compare_id;
        } else {
            return task_compare_id_reverse;
        }
    } else {
        if (ascending) {
            return task_compare_priority;
        } else {
            return task_compare_priority_reverse;
        }
    }
    UNREACHABLE("task_sorter");
}

typedef struct Command Command;

struct Command {
    const char *name;
    const char *description;
    const char *signature;
    bool (*run)(Command *self, const char *program_name, int argc, char **argv);
};

static void print_command_usage(Command *command, const char *program_name, void *c)
{
    fprintf(stderr, "Usage: %s %s %s\n", program_name, command->name, command->signature);
    fprintf(stderr, "OPTIONS:\n");
    flag_c_print_options(c, stderr);
}

static bool init_run(Command *self, const char *program_name, int argc, char **argv)
{
    UNUSED(self);
    UNUSED(program_name);
    UNUSED(argc);
    UNUSED(argv);
    if (!mkdir_if_not_exists("./tasks/")) return false;
    return true;
}

bool task_matches_tags(const Task *task, const char **tags, size_t tags_count)
{
    for (size_t j = 0; j < tags_count; ++j) {
        if (!tags_contains(task->tags, sv_from_cstr(tags[j]))) {
            return false;
        }
    }
    return true;
}

typedef enum {
    OP_ANY,
    OP_TAG,
    OP_NOT,
    OP_OR,
    OP_AND,
    OP_TAGGED,
} Op_Kind;

typedef struct {
    Op_Kind kind;
    String_View tag;
} Op;

void print_op(Op op)
{
    switch (op.kind) {
    case OP_ANY:      printf("OP_ANY\n");                          break;
    case OP_TAG:      printf("OP_TAG "SV_Fmt"\n", SV_Arg(op.tag)); break;
    case OP_NOT:      printf("OP_NOT\n");                          break;
    case OP_OR:       printf("OP_OR\n");                           break;
    case OP_AND:      printf("OP_AND\n");                          break;
    case OP_TAGGED:   printf("OP_TAGGED\n");                       break;
    default: UNREACHABLE("Op_Kind");
    }
}

typedef struct {
    Op *items;
    size_t count;
    size_t capacity;
} Filter;

typedef struct {
    bool *items;
    size_t count;
    size_t capacity;
} Stack;

static bool task_matches_filter(const Task *task, Filter filter, Stack *stack)
{
    stack->count = 0;
    da_foreach(Op, op, &filter) {
        switch (op->kind) {
        case OP_ANY: {
            da_append(stack, true);
        } break;
        case OP_TAG: {
            bool result = tags_contains(task->tags, op->tag);
            da_append(stack, result);
        } break;
        case OP_NOT: {
            da_last(stack) = !da_last(stack);
        } break;
        case OP_OR: {
            bool a = da_pop(stack);
            bool b = da_pop(stack);
            da_append(stack, a || b);
        } break;
        case OP_AND: {
            bool a = da_pop(stack);
            bool b = da_pop(stack);
            da_append(stack, a && b);
        } break;
        case OP_TAGGED: {
            da_append(stack, task->tags.count > 0);
        } break;
        default: UNREACHABLE("Op_Kind");
        }
    }
    return da_first(stack);
}

static int not_end_of_filter_token(int x)
{
    return x != ' ' && x != ')' && x != '(';
}

static void report_compile_filter_error(String_View original_src, const String_View *src, const char *format, ...) NOB_PRINTF_FORMAT(3, 4);
static void report_compile_filter_error(String_View original_src, const String_View *src, const char *format, ...)
{
    int cursor = sv_utf8_len((String_View) {
        .data = original_src.data,
        .count = src->data - original_src.data,
    }, NULL) + 1;
    fprintf(stderr, SV_Fmt"\n", SV_Arg(original_src));
    fprintf(stderr, "%*s\n", cursor, "^");
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static bool compile_filter_expr(String_View original_src, String_View *src, Filter *filter);

static bool compile_filter_primary(String_View original_src, String_View *src, Filter *filter)
{
    *src = sv_trim_left(*src);
    if (src->count == 0) {
        report_compile_filter_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
        return false;
    }
    if (*src->data == '.') {
        sv_chop_left(src, 1);
        String_View tag = sv_chop_while(src, not_end_of_filter_token);
        da_append(filter, ((Op) {
            .kind = OP_TAG,
            .tag = tag,
        }));
        return true;
    }
    if (*src->data == '(') {
        sv_chop_left(src, 1);
        if (!compile_filter_expr(original_src, src, filter)) return false;
        *src = sv_trim_left(*src);
        if (!sv_starts_with(*src, sv_from_cstr(")"))) {
            report_compile_filter_error(original_src, src, "Expected `)`.");
            return false;
        }
        sv_chop_left(src, 1);
        return true;
    }
    String_View saved_src = *src;
    String_View key = sv_chop_while(src, not_end_of_filter_token);
    if (sv_eq(key, sv_from_cstr("not"))) {
        if (!compile_filter_primary(original_src, src, filter)) return false;
        da_append(filter, ((Op) { .kind = OP_NOT }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("any"))) {
        da_append(filter, ((Op) { .kind = OP_ANY }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("tagged"))) {
        da_append(filter, ((Op) { .kind = OP_TAGGED }));
        return true;
    }
    *src = saved_src;
    if (key.count == 0) {
        report_compile_filter_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
    } else {
        report_compile_filter_error(original_src, src, "Unknown keyword `"SV_Fmt"`. Expected keywords `not`, `any`, or `tagged`.", SV_Arg(key));
    }
    return false;
}

static bool compile_filter_and(String_View original_src, String_View *src, Filter *filter)
{
    // <expr> [and <expr>]*
    if (!compile_filter_primary(original_src, src, filter)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_filter_token);
        if (!sv_eq(key, sv_from_cstr("and"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_filter_primary(original_src, src, filter)) return false;
        da_append(filter, ((Op) {.kind = OP_AND}));
    }
    return true;
}

static bool compile_filter_or(String_View original_src, String_View *src, Filter *filter)
{
    // <expr> [or <expr>]*
    if (!compile_filter_and(original_src, src, filter)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_filter_token);
        if (!sv_eq(key, sv_from_cstr("or"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_filter_and(original_src, src, filter)) return false;
        da_append(filter, ((Op) {.kind = OP_OR}));
    }
    return true;
}

static bool compile_filter_expr(String_View original_src, String_View *src, Filter *filter)
{
    if (!compile_filter_or(original_src, src, filter)) return false;
    return true;
}

static bool compile_filter(String_View original_src, String_View *src, Filter *filter)
{
    if (!compile_filter_expr(original_src, src, filter)) return false;
    *src = sv_trim_left(*src);
    if (src->count != 0) {
        report_compile_filter_error(original_src, src, "Expected keywords `and`, or `or`");
        return false;
    }
    return true;
}

static bool ls_run(Command *self, const char *program_name, int argc, char **argv)
{
    bool closed = false;
    bool ascending = false;
    bool help = false;
    bool debug = false;
    bool by_id = false;
    String_Builder filter_src = {0};

    void *c = flag_c_new(program_name);
    flag_c_bool_var(c, &closed, "c", false, "List closed tasks");
    flag_c_bool_var(c, &ascending, "a", false, "List tasks in ascending order");
    flag_c_bool_var(c, &by_id, "id", false, "Sort tasks by id");
    flag_c_bool_var(c, &debug, "debug", false, "Output opcodes of the filter for debug purpose");
    flag_c_bool_var(c, &help, "help", false, "Print this help message");

    while (argc > 0) {
        if (!flag_c_parse(c, argc, argv)) {
            print_command_usage(self, program_name, c);
            flag_c_print_error(c, stderr);
            return false;
        }

        argc = flag_c_rest_argc(c);
        argv = flag_c_rest_argv(c);

        if (argc > 0) {
            if (filter_src.count > 0) sb_append(&filter_src, ' ');
            sb_append_cstr(&filter_src, shift(argv, argc));
        }
    }

    if (help) {
        print_command_usage(self, program_name, c);
        return true;
    }

    String_View src = sv_trim(sb_to_sv(filter_src));
    if (src.count == 0) src = sv_from_cstr("any");
    String_View original_src = src;
    Filter filter = {0};
    if (!compile_filter(original_src, &src, &filter)) return false;

    if (debug) {
        da_foreach(Op, op, &filter) {
            print_op(*op);
        }
        return true;
    }

    Path dir_path = {0};
    if (!find_tasks_database(&dir_path)) return false;

    Path cwd_path = {0};
    if (!get_current_dir_path(&cwd_path)) return false;

    Path rel_path = {0};
    path_relative(&rel_path, cwd_path, dir_path);

    String_Builder sb = {0};

    Tasks tasks = {0};
    if (!load_tasks(&tasks, path_render_cstr(&sb, dir_path))) return false;
    qsort(tasks.items, tasks.count, sizeof(*tasks.items), task_sorter(by_id, ascending));

    Stack stack = {0};

    size_t tasks_matched = 0;
    da_foreach(Task, task, &tasks) {
        if (!sv_eq(task->status, closed ? SVLIT("CLOSED") : SVLIT("OPEN"))) continue;
        if (!task_matches_filter(task, filter, &stack)) continue;
        print_task(path_render_cstr(&sb, rel_path), task);
        tasks_matched += 1;
    }

    if (tasks_matched == 0) {
        printf("No tasks were found\n");
    }

    return true;
}

static bool new_run(Command *self, const char *program_name, int argc, char **argv)
{
    Flag_List tags = {0};
    bool help = false;
    uint64_t priority = 0;

    void *c = flag_c_new(program_name);
    flag_c_list_var(c, &tags, "t", "Tags to add to the new task");
    flag_c_uint64_var(c, &priority, "p", DEFAULT_PRIORITY, "Priority of the new task");
    flag_c_bool_var(c, &help, "help", false, "Print this help message");
    String_Builder sb_title = {0};

    while (argc > 0) {
        if (!flag_c_parse(c, argc, argv)) {
            print_command_usage(self, program_name, c);
            flag_c_print_error(c, stderr);
            return false;
        }

        argc = flag_c_rest_argc(c);
        argv = flag_c_rest_argv(c);

        if (argc > 0) {
            if (sb_title.count > 0) sb_append(&sb_title, ' ');
            sb_append_cstr(&sb_title, shift(argv, argc));
        }
    }

    if (help) {
        print_command_usage(self, program_name, c);
        return true;
    }

    String_Builder sb_dir_path = {0};

    Path dir_path = {0};
    if (!find_tasks_database(&dir_path)) return false;

    Path cwd_path = {0};
    if (!get_current_dir_path(&cwd_path)) return false;

    Path rel_path = {0};
    path_relative(&rel_path, cwd_path, dir_path);

    char *id = temp_new_huid();
    const char *task_path = temp_sprintf("%s/%s", path_render_cstr(&sb_dir_path, dir_path), id);
    int exists = file_exists(task_path);
    if (exists < 0) return false;
    if (exists) {
        nob_log(ERROR, "%s already exists. You are probably creating tasks too fast, or your time is broken", id);
        return false;
    }
    if (!mkdir_if_not_exists(task_path)) return false;
    String_View title = SVLIT(DEFAULT_TASK_TITLE);
    if (sb_title.count > 0) {
        title = sb_to_sv(sb_title);
    }

    Task task = {
        .id       = id,
        .title    = title,
        .status   = SVLIT("OPEN"),
        .priority = priority,
    };

    da_foreach(const char *, tag, &tags) {
        da_append(&task.tags, sv_from_cstr(*tag));
    }

    String_Builder sb_md_content = {0};

    append_task_md_content(&sb_md_content, task);
    const char *task_md_path = temp_sprintf("%s/%s/TASK.md", path_render_cstr(&sb_dir_path, dir_path), id);
    if (!write_entire_file(task_md_path, sb_md_content.items, sb_md_content.count)) return false;

    String_Builder sb_rel_path = {0};
    print_task(path_render_cstr(&sb_rel_path, rel_path), &task);
    return true;
}

static bool find_run(Command *self, const char *program_name, int argc, char **argv)
{
    bool help = false;
    bool path_only = false;

    void *c = flag_c_new(program_name);
    flag_c_bool_var(c, &path_only, "path-only", false, "Print only path to TASK.md");
    flag_c_bool_var(c, &help, "help", false, "Print this help message");

    const char *huid = NULL;

    while (argc > 0) {
        if (!flag_c_parse(c, argc, argv)) {
            print_command_usage(self, program_name, c);
            flag_c_print_error(c, stderr);
            return false;
        }

        argc = flag_c_rest_argc(c);
        argv = flag_c_rest_argv(c);

        if (argc > 0) {
            if (huid != NULL) {
                print_command_usage(self, program_name, c);
                nob_log(ERROR, "Several HUIDs is not supported");
                return false;
            }

            huid = shift(argv, argc);
            if (!is_valid_huid(huid)) {
                print_command_usage(self, program_name, c);
                nob_log(ERROR, "`%s` is not a valid HUID. Valid HUID matches regexp %s", huid, HUID_REGEXP_FOR_USER_REPORT_PURPOSES);
                return false;
            }
        }
    }

    if (help) {
        print_command_usage(self, program_name, c);
        return true;
    }

    if (huid == NULL) {
        print_command_usage(self, program_name, c);
        nob_log(ERROR, "No HUID was provided");
        return false;
    }

    Path dir_path = {0};
    if (!find_tasks_database(&dir_path)) return false;

    Path cwd_path = {0};
    if (!get_current_dir_path(&cwd_path)) return false;

    Path rel_path = {0};
    path_relative(&rel_path, cwd_path, dir_path);

    String_Builder sb_path = {0};

    Tasks tasks = {0};
    if (!load_tasks(&tasks, path_render_cstr(&sb_path, dir_path))) return false;

    bool found = false;
    da_foreach(Task, task, &tasks) {
        if (strcmp(task->id, huid) == 0) {
            if (path_only) {
                printf("%s/%s/TASK.md\n", path_render_cstr(&sb_path, rel_path), task->id);
            } else {
                print_task(path_render_cstr(&sb_path, rel_path), task);
            }
            found = true;
        }
    }
    if (!found) {
        nob_log(ERROR, "No task with with HUID `%s` was found", huid);
        return false;
    }
    return true;
}

static bool graph_run(Command *self, const char *program_name, int argc, char **argv)
{
    UNUSED(self);
    UNUSED(program_name);
    UNUSED(argc);
    UNUSED(argv);

    Path dir_path = {0};
    if (!find_tasks_database(&dir_path)) return false;

    String_Builder sb_path = {0};

    Tasks tasks = {0};
    if (!load_tasks(&tasks, path_render_cstr(&sb_path, dir_path))) return false;

    typedef Ht(String_View, bool) HUIDs;
    Ht(const char *, HUIDs) graph = {
        .hasheq = ht_cstr_hasheq,
        .default_value = {
            .hasheq = ht_sv_hasheq,
        }
    };
    da_foreach(Task, task, &tasks) {
        HUIDs *ref = ht_put(&graph, task->id);
        String_View content = task->task_md_content;
        while (content.count > 0) {
            String_View huid = {0};
            if (chop_huid(&content, &huid)) {
                *ht_put(ref, huid) = true;
            } else {
                sv_chop_left(&content, 1);
            }
        }
    }

    String_Builder sb = {0};

    sb_appendf(&sb, "digraph {\n");
    ht_foreach(ref, &graph) {
        const char *orig = ht_key(&graph, ref);
        if (ref->count == 0) continue;
        ht_foreach(huid, ref) {
            String_View nbor = ht_key(ref, huid);
            sb_appendf(&sb, "    \"%s\" -> \""SV_Fmt"\";\n", orig, SV_Arg(nbor));
        }
    }
    sb_appendf(&sb, "}\n");

    const char *format = "svg";
    const char *dot_path = "graph.dot";
    const char *out_path = temp_sprintf("graph.%s", format);
    if (!write_entire_file(dot_path, sb.items, sb.count)) return false;
    nob_log(INFO, "Generated %s", dot_path);

    Cmd cmd = {0};
    // cmd_append(&cmd, "dot");
    cmd_append(&cmd, "neato");
    // cmd_append(&cmd, "twopi");
    cmd_append(&cmd, "-Goverlap=scale");
    cmd_append(&cmd, temp_sprintf("-T%s", format));
    cmd_append(&cmd, dot_path);
    cmd_append(&cmd, temp_sprintf("-o%s", out_path));
    if (!cmd_run(&cmd)) return false;

    return true;
}

typedef struct {
    String_View tag;
    size_t count;
} Tag_Count;

int tag_count_compare_by_count_desc(const void *a, const void *b)
{
    const Tag_Count *tca = a;
    const Tag_Count *tcb = b;
    if (tca->count < tcb->count) return 1;
    if (tca->count > tcb->count) return -1;
    return 0;
}

static bool summary_run(Command *self, const char *program_name, int argc, char **argv)
{
    bool closed = false;
    bool help = false;
    void *c = flag_c_new(program_name);
    flag_c_bool_var(c, &closed, "c", false, "List closed tasks");
    flag_c_bool_var(c, &help, "help", false, "Print this help message");

    if (!flag_c_parse(c, argc, argv)) {
        print_command_usage(self, program_name, c);
        flag_c_print_error(c, stderr);
        return false;
    }

    if (help) {
        print_command_usage(self, program_name, c);
        return true;
    }

    String_Builder sb_path = {0};

    Path dir_path = {0};
    if (!find_tasks_database(&dir_path)) return false;

    Tasks tasks = {0};
    if (!load_tasks(&tasks, path_render_cstr(&sb_path, dir_path))) return false;

    Ht(String_View, String_View) tags_desc = {
        .hasheq = ht_sv_hasheq,
    };
    const char *tags_desc_path = temp_sprintf("%s/tags", path_render_cstr(&sb_path, dir_path));
    if (file_exists(tags_desc_path)) {
        String_Builder sb = {0};
        if (!read_entire_file(tags_desc_path, &sb)) return false;
        String_View sv = sb_to_sv(sb);
        for (size_t line_number = 0; sv.count > 0; ++line_number) {
            String_View line = sv_trim(sv_chop_by_delim(&sv, '\n'));
            if (line.count == 0) continue;
            String_View tag  = sv_trim(sv_chop_by_delim(&line, ','));
            String_View desc = sv_trim(line);
            String_View *slot = ht_find(&tags_desc, tag);
            if (slot) {
                fprintf(stderr, "%s:%zu: WARNING: redefinition of tag description '"SV_Fmt"'\n", tags_desc_path, line_number, SV_Arg(tag));
            } else {
                slot = ht_put(&tags_desc, tag);
            }
            *slot = desc;
        }
    }

    size_t total_count = 0;
    size_t untagged_count = 0;
    Ht(String_View, size_t) tags_count = {
        .hasheq = ht_sv_hasheq,
    };
    for (size_t i = 0; i < tasks.count; ++i) {
        Task *task = &tasks.items[i];
        bool task_is_closed = sv_eq(task->status, SVLIT("CLOSED"));
        if (closed == task_is_closed) {
            total_count += 1;
            for (size_t j = 0; j < task->tags.count; ++j) {
                *ht_find_or_put(&tags_count, task->tags.items[j]) += 1;
            }
            if (task->tags.count == 0) {
                untagged_count += 1;
            }
        }
    }

    struct {
        Tag_Count *items;
        size_t count;
        size_t capacity;
    } sorted_tags_count = {0};

    size_t max_width = 0;
    ht_foreach(value, &tags_count) {
        String_View key = ht_key(&tags_count, value);
        if (max_width < key.count) {
            max_width = key.count;
        }
        da_append(&sorted_tags_count, ((Tag_Count) {
            .tag = key,
            .count = *value,
        }));
    }

    qsort(sorted_tags_count.items, sorted_tags_count.count, sizeof(*sorted_tags_count.items), tag_count_compare_by_count_desc);

    const char *status = closed ? "CLOSED" : "OPEN";
    printf("STATUS:   %s\n",  status);
    printf("TOTAL:    %zu\n", total_count);
    if (untagged_count > 0) {
        printf("UNTAGGED: %zu\n", untagged_count);
    }
    if (sorted_tags_count.count > 0) {
        printf("TAGGED:\n");
        size_t mark = temp_save();
        da_foreach(Tag_Count, tag_count, &sorted_tags_count) {
            temp_rewind(mark);
            String_View *tag_desc = ht_find(&tags_desc, tag_count->tag);
            if (tag_desc) {
                printf("    %*s => %3zu - "SV_Fmt"\n", (int)max_width, temp_sv_to_cstr(tag_count->tag), tag_count->count, SV_Arg(*tag_desc));
            } else {
                printf("    %*s => %3zu\n", (int)max_width, temp_sv_to_cstr(tag_count->tag), tag_count->count);
            }
        }
    }

    return true;
}

static bool help_run(Command *self, const char *program_name, int argc, char **argv);

static bool version_run(Command *self, const char *program_name, int argc, char **argv)
{
    UNUSED(self);
    UNUSED(program_name);
    UNUSED(argc);
    UNUSED(argv);
    printf("tatr - Task Tracker\n");
    printf("Built at %s\n", BUILD_TIME);
    printf("GIT HASH: "GIT_HASH"\n");
    return true;
}

static Command commands[] = {
    {
        .name = "init",
        .description = "Create tasks/ directory in the current working directory if it doesn't exist yet",
        .run = init_run,
    },
    {
        .name = "ls",
        .signature = "[OPTIONS]",
        .description = "List the tasks",
        .run = ls_run,
    },
    {
        .name = "new",
        .signature = "[TITLE...] [OPTIONS]",
        .description = "Create a new task",
        .run = new_run,
    },
    {
        .name = "summary",
        .signature = "[OPTIONS]",
        .description = "Print the summary of the tasks",
        .run = summary_run,
    },
    {
        .name = "find",
        .signature = "<HUID> [OPTIONS]",
        .description = "Find the task with a given HUID",
        .run = find_run,
    },
    {
        .name = "graph",
        .description = "Generate graph of tasks cross-referring to each other",
        .run = graph_run,
    },
    {
        .name = "help",
        .signature = "[OPTIONS]",
        .description = "Print this help message",
        .run = help_run,
    },
    {
        .name = "version",
        .description = "Print the current version and date of the build",
        .run =  version_run,
    },
};

static void print_available_commands(Log_Level log_level)
{
    nob_log(log_level, "Available commands:");
    int max_width = 0;
    for (size_t i = 0; i < ARRAY_LEN(commands); ++i) {
        int width = strlen(commands[i].name);
        if (width > max_width) max_width = width;
    }
    for (size_t i = 0; i < ARRAY_LEN(commands); ++i) {
        nob_log(log_level, "    %-*s - %s", max_width, commands[i].name, commands[i].description);
    }
}

static bool help_run(Command *self, const char *program_name, int argc, char **argv)
{
    UNUSED(self);
    UNUSED(program_name);
    UNUSED(argc);
    UNUSED(argv);
    print_available_commands(INFO);
    return true;
}
#ifndef TASKS_TEST
int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);

    if (argc <= 0) {
        print_available_commands(ERROR);
        nob_log(ERROR, "No command is provided");
        return 0;
    }

    const char *command_name = shift(argv, argc);

    for (size_t i = 0; i < ARRAY_LEN(commands); ++i) {
        if (strcmp(command_name, commands[i].name) == 0) {
            if (!commands[i].run(&commands[i], program_name, argc, argv)) return 1;
            return 0;
        }
    }

    print_available_commands(ERROR);
    nob_log(ERROR, "Unknown command `%s`", command_name);
    return 1;
}
#else
// TASK(20260825-195942): Move the relative path test into a separate unit
int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    static struct {
        const char *description;
        const char *dst_path;
        const char *src_path;
        const char *rel_path;
    } cases[] = {
        {
            .description = "At the root of the project",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren",
            .rel_path = "./tasks",
        },
        {
            .description = "At the root of the project, leading slash in dst_path",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks/",
            .src_path = "/home/rexim/Programming/tsoding/sofren",
            .rel_path = "./tasks",
        },
        {
            .description = "Same directory",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = ".",
        },
        {
            .description = "Same directory deeper",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks/20250831-161356/",
            .rel_path = "..",
        },
        {
            .description = "Sibling directory",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/src",
            .rel_path = "../tasks",
        },
        {
            .description = "Sibling directory deeper",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/src/game/",
            .rel_path = "../../tasks",
        },
        {
            .description = "Completely different path",
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home_/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = "../../../../../../home/rexim/Programming/tsoding/sofren/tasks",
        },
        {
            .description = "20260321-181305",
            .dst_path = "/home/rexim/Programming/tsoding/tatr/tasks",
            .src_path = "/home/rexim/Programming/tsoding/tatr/thirdparty",
            .rel_path = "../tasks",
        },
    };

    Path dst_path          = {0};
    Path src_path          = {0};
    Path rel_path_expected = {0};
    Path rel_path_actual   = {0};
    String_Builder sb_path = {0};
    bool record = false;
    if (!record) {
        // Replay
        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            const char *description = cases[i].description;
            path_parse(&dst_path,          sv_from_cstr(cases[i].dst_path));
            path_parse(&src_path,          sv_from_cstr(cases[i].src_path));
            path_parse(&rel_path_expected, sv_from_cstr(cases[i].rel_path));
            printf("%s ...", description);
            fflush(stdout);
            path_relative(&rel_path_actual, src_path, dst_path);
            if (path_eq(rel_path_actual, rel_path_expected)) {
                printf(" OK\n");
            } else {
                printf(" FAILED\n");
                printf("  EXPECTED: %s\n", path_render_cstr(&sb_path, rel_path_expected));
                printf("  ACTUAL:   %s\n", path_render_cstr(&sb_path, rel_path_actual));
                abort();
            }
        }
    } else {
        // Record
        printf("{\n");
        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            const char *description = cases[i].description;
            path_parse(&dst_path, sv_from_cstr(cases[i].dst_path));
            path_parse(&src_path, sv_from_cstr(cases[i].src_path));
            path_relative(&rel_path_actual, src_path, dst_path);
            printf("    {\n");
            printf("        .description = \"%s\",\n", description);
            printf("        .dst_path    = \"%s\",\n", path_render_cstr(&sb_path, dst_path));
            printf("        .src_path    = \"%s\",\n", path_render_cstr(&sb_path, src_path));
            printf("        .rel_path    = \"%s\",\n", path_render_cstr(&sb_path, rel_path_actual));
            printf("    },\n");
        }
        printf("}\n");
    }
    return 0;
}
#endif // TASKS_TEST

#define NOB_IMPLEMENTATION
#define NOB_OVERWRITE_TEMP_ON_REWIND
#include "nob.h"
#define FLAG_IMPLEMENTATION
#define FLAG_PUSH_DASH_DASH_BACK
#include "flag.h"
#define HT_IMPLEMENTATION
#include "ht.h"

#include "path.c"
#include "huid.c"
#include "md.c"
