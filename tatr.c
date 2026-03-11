// Simple tool for manipulating tasks database
#define NOB_IMPLEMENTATION
#include "./thirdparty/nob.h"
#define FLAG_IMPLEMENTATION
#define FLAG_PUSH_DASH_DASH_BACK
#include "./thirdparty/flag.h"
#define HT_IMPLEMENTATION
#include "./thirdparty/ht.h"

#define DEFAULT_TASK_TITLE "New Task"
#define DEFAULT_PRIORITY 100

#define HUID_REGEXP_FOR_USER_REPORT_PURPOSES "/[0-9]{8}-[0-9]{6}/"
static inline bool is_valid_huid(const char *id)
{
    for (int i = 0; i < 8; ++i) if (!isdigit(*id++)) return false;
    if (*id++ != '-')                                return false;
    for (int i = 0; i < 6; ++i) if (!isdigit(*id++)) return false;
    if (*id++ != '\0')                               return false;
    return true;
}

static inline bool isspace_except_newline(char x)
{
    return isspace(x) && x != '\n' && x != '\r';
}

typedef struct {
    char *file;
    char *source;
    size_t cur, bol, line;
} Md;

static inline void md_init(Md *md, char *file, char *source)
{
    memset(md, 0, sizeof(*md));
    md->file = file;
    md->source = source;
    md->line = 1;
}

static inline size_t md_col(Md *md)
{
    return md->cur - md->bol + 1;
}

static inline char *md_cstr(Md *md)
{
    return &md->source[md->cur];
}

static inline char md_char(Md *md)
{
    return md->source[md->cur];
}

static inline bool md_end(Md *md)
{
    return md_char(md) == '\0';
}

static char md_next_char(Md *md)
{
    if (md_end(md)) return '\0';
    char x = md->source[md->cur++];
    if (x == '\n') {
        md->bol = md->cur;
        md->line += 1;
    }
    return x;
}

static inline bool md_expect_char(Md *md, char x)
{
    if (md_char(md) != x) return false;
    md_next_char(md);
    return true;
}

static bool md_expect_str(Md *md, char *str)
{
    size_t saved_cur = md->cur;
    while (!md_end(md) && *str) {
        if (md_next_char(md) != *str++) {
            md->cur = saved_cur;
            return false;
        }
    }
    return true;
}

static void md_trim_spaces(Md *md)
{
    while (isspace(md_char(md))) {
        md_next_char(md);
    }
}

static void md_trim_spaces_except_newline(Md *md)
{
    while (isspace_except_newline(md_char(md))) {
        md_next_char(md);
    }
}

static char *md_dup_text_until_newline(Md *md)
{
    char *start = md_cstr(md);
    while (!md_end(md) && md_char(md) != '\n') {
        md_next_char(md);
    }
    return strndup(start, md_cstr(md) - start);
}

static char *task_md_extract_title(Md *md)
{
    md_trim_spaces(md);
    if (!md_expect_char(md, '#')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected '#' as a start of a title, but got %c\n", md->file, md->line, md_col(md), md_char(md));
        return NULL;
    }
    md_trim_spaces_except_newline(md);
    return md_dup_text_until_newline(md);
}

static char *task_md_extract_field(Md *md, char *field)
{
    md_trim_spaces(md);
    if (!md_expect_char(md, '-')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected '-' as a start of a TASK.md field '%s', but got %c\n", md->file, md->line, md_col(md), field, md_char(md));
        return NULL;
    }
    md_trim_spaces_except_newline(md);
    if (!md_expect_str(md, field)) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected a TASK.md field name '%s'\n", md->file, md->line, md_col(md), field);
        return NULL;
    }
    md_trim_spaces_except_newline(md);
    if (!md_expect_char(md, ':')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected a field separator ':'\n", md->file, md->line, md_col(md));
        return NULL;
    }
    md_trim_spaces_except_newline(md);
    return md_dup_text_until_newline(md);
}

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
    const char *id;
    const char *title;
    const char *status;
    Tags tags;
    int priority;
} Task;

static void append_task_md_content(String_Builder *sb, Task task)
{
    sb_appendf(sb, "# %s\n", task.title);
    sb_appendf(sb, "\n");
    sb_appendf(sb, "- STATUS: %s\n", task.status);
    sb_appendf(sb, "- PRIORITY: %d\n", task.priority);
    if (task.tags.count > 0) {
        sb_appendf(sb, "- TAGS: ");
        for (size_t i = 0; i < task.tags.count; ++i) {
            if (i > 0) sb_appendf(sb, ",");
            sb_append_buf(sb, task.tags.items[i].data, task.tags.items[i].count);
        }
        sb_appendf(sb, "\n");
    }
    sb_appendf(sb, "\n");
    sb_appendf(sb, "No description.\n");
}

// TASK(20260308-163429): Tasks array should be a hash table
typedef struct {
    Task *items;
    size_t count;
    size_t capacity;
} Tasks;

static void print_task(const char *rel_path, Task *task)
{
    if (task->tags.count) {
        static String_Builder sb = {0};
        sb.count = 0;
        for (size_t i = 0; i < task->tags.count; ++i) {
            if (i > 0) sb_appendf(&sb, ",");
            sb_appendf(&sb, SV_Fmt, SV_Arg(task->tags.items[i]));
        }
        sb_append_null(&sb);
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d, TAGS: %s] %s\n", rel_path, task->id, task->priority, sb.items, task->title);
    } else {
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d] %s\n", rel_path, task->id, task->priority, task->title);
    }
}

static const char *find_tasks_database(void)
{
    const char *dir = get_current_dir_temp();
    if (!dir) return NULL;
    while (strcmp(dir, "/") != 0) {
        const char *path = temp_sprintf("%s/tasks", dir);
        int exists = nob_file_exists(path);
        if (exists < 0) return NULL;
        if (exists) {
            File_Type type = get_file_type(path);
            if (type < 0) return NULL;
            if (type == FILE_DIRECTORY) return path;
        }
        dir = temp_dir_name(dir);
    }
    nob_log(ERROR, "Could not find tasks/ folder");
    return NULL;
}

static bool load_tasks(Tasks *tasks, const char *dir_path)
{
    File_Paths children = {0};
    String_Builder sb = {0};
    Md md = {0};

    if (!read_entire_dir(dir_path, &children)) return false;
    for (size_t i = 0; i < children.count; ++i) {
        const char *id = children.items[i];
        if (*id == '.') continue;
        if (!is_valid_huid(id)) continue;
        const char *task_path = temp_sprintf("%s/%s", dir_path, id);
        File_Type type = get_file_type(task_path);
        if (type < 0) return false;
        if (type != FILE_DIRECTORY) {
            nob_log(ERROR, "%s is not a directory", id);
            return false;
        }
        sb.count = 0;
        const char *task_md_path = temp_sprintf("%s/%s/TASK.md", dir_path, id);
        // TASK(20260308-171346): there should be a command that reports all the skipped weird folders and files found in the tasks/ folder
        if (!file_exists(task_md_path)) continue; // Ignore task folders without TASK.md
        if (!read_entire_file(task_md_path, &sb)) return false;
        sb_append_null(&sb);
        md_init(&md, (char*)task_md_path, sb.items);

        char *title = task_md_extract_title(&md);
        if (title == NULL) return false;
        char *status = task_md_extract_field(&md, "STATUS");
        if (status == NULL) return false;
        char *priority = task_md_extract_field(&md, "PRIORITY");
        if (priority == NULL) return false;
        Tags tags = {0};
        md_trim_spaces(&md);
        if (md_char(&md) == '-') {
            char *tags_string = task_md_extract_field(&md, "TAGS");
            if (tags_string == NULL) return false;
            String_View sv = sv_trim_left(sv_from_cstr(tags_string));
            while (sv.count > 0) {
                String_View tag = sv_trim(sv_chop_by_delim(&sv, ','));
                da_append(&tags, tag);
                sv = sv_trim_left(sv);
            }
        }

        da_append(tasks, ((Task) {
            .id         = strdup(id),
            .title      = title,
            .status     = status,
            .priority   = atoi(priority),
            .tags       = tags,
        }));
    }

    return true;
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

typedef struct Command Command;

struct Command {
    const char *name;
    const char *description;
    const char *signature;
    bool (*run)(Command *self, const char *program_name, int argc, char **argv);
};

// Both dst_path and src_path are expected to be absolute
// TODO: Would be great to normalize the paths before processing them
static const char *relative_path(const char *dst_path, const char *src_path)
{
    while (*dst_path && *src_path && *dst_path == *src_path) {
        dst_path++;
        src_path++;
    }

    size_t backtrack = 0;
    while (src_path && *src_path == '/') src_path++;
    while (src_path && *src_path) {
        backtrack += 1;
        src_path = strchr(src_path, '/');
        while (src_path && *src_path == '/') src_path++;
    }

    while (dst_path && *dst_path == '/') dst_path++;
    size_t dst_len = strlen(dst_path);

    if (backtrack > 0) {
        const char *up = "../";
        size_t up_len = strlen(up);
        size_t result_len = backtrack*up_len + dst_len;
        char *result = temp_alloc(result_len + 1);
        for (size_t i = 0; i < backtrack; ++i) {
            memcpy(result + i*up_len, up, up_len);
        }
        memcpy(result + backtrack*up_len, dst_path, dst_len);
        result[result_len] = '\0';
        while (result_len > 0 && result[result_len-1] == '/') {
            result[--result_len] = '\0';
        }
        return result;
    }

    const char *here = "./";
    size_t here_len = strlen(here);
    size_t result_len = here_len + dst_len;
    char *result = temp_alloc(result_len + 1);
    memcpy(result, here, here_len);
    memcpy(result + here_len, dst_path, dst_len);
    result[result_len] = '\0';
    while (result_len > 0 && result[result_len-1] == '/') {
        result[--result_len] = '\0';
    }
    return result;
}

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
    int cursor = src->data - original_src.data + 1;
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
    bool debug_filter = false;
    char *filter_src = NULL;

    void *c = flag_c_new(program_name);
    flag_c_bool_var(c, &closed, "c", false, "List closed tasks");
    flag_c_bool_var(c, &ascending, "a", false, "List tasks in ascending order");
    flag_c_str_var(c, &filter_src, "f", "any", "A filter to filter the tasks by");
    flag_c_bool_var(c, &debug_filter, "df", false, "Output opcodes of the filter for debug purpose");
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

    Filter filter = {0};
    String_View original_src = sv_from_cstr(filter_src);
    String_View src          = sv_from_cstr(filter_src);
    if (!compile_filter(original_src, &src, &filter)) return false;

    if (debug_filter) {
        da_foreach(Op, op, &filter) {
            print_op(*op);
        }
        return true;
    }

    const char *dir_path = find_tasks_database();
    if (!dir_path) return false;

    const char *cwd_path = get_current_dir_temp();
    if (!cwd_path) return false;

    const char *rel_path = relative_path(dir_path, cwd_path);

    Tasks tasks = {0};
    if (!load_tasks(&tasks, dir_path)) return false;
    if (ascending) {
        qsort(tasks.items, tasks.count, sizeof(*tasks.items), task_compare_priority);
    } else {
        qsort(tasks.items, tasks.count, sizeof(*tasks.items), task_compare_priority_reverse);
    }

    Stack stack = {0};

    size_t tasks_matched = 0;
    da_foreach(Task, task, &tasks) {
        if (strcmp(task->status, closed ? "CLOSED" : "OPEN") != 0) continue;
        if (!task_matches_filter(task, filter, &stack)) continue;
        print_task(rel_path, task);
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

    const char *dir_path = find_tasks_database();
    if (!dir_path) return false;

    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = gmtime(&rawtime);
    const char *id = temp_sprintf("%04d%02d%02d-%02d%02d%02d",
        timeinfo->tm_year+1900,
        timeinfo->tm_mon+1,
        timeinfo->tm_mday,
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec);
    const char *task_path = temp_sprintf("%s/%s", dir_path, id);
    int exists = file_exists(task_path);
    if (exists < 0) return false;
    if (exists) {
        nob_log(ERROR, "%s already exists. You are probably creating tasks too fast, or your time is broken", id);
        return false;
    }
    if (!mkdir_if_not_exists(task_path)) return false;
    String_Builder sb = {0};
    const char *title = DEFAULT_TASK_TITLE;
    if (sb_title.count > 0) {
        sb_append_null(&sb_title);
        title = sb_title.items;
    }

    Task task = {
        .id       = id,
        .title    = title,
        .status   = "OPEN",
        .priority = priority,
    };

    da_foreach(const char *, tag, &tags) {
        da_append(&task.tags, sv_from_cstr(*tag));
    }

    append_task_md_content(&sb, task);
    const char *task_md_path = temp_sprintf("%s/%s/TASK.md", dir_path, id);
    if (!write_entire_file(task_md_path, sb.items, sb.count)) return false;

    const char *cwd_path = get_current_dir_temp();
    if (!cwd_path) return false;

    const char *rel_path = relative_path(dir_path, cwd_path);

    print_task(rel_path, &task);
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

    const char *dir_path = find_tasks_database();
    if (!dir_path) return false;

    const char *cwd_path = get_current_dir_temp();
    if (!cwd_path) return false;

    const char *rel_path = relative_path(dir_path, cwd_path);

    Tasks tasks = {0};
    if (!load_tasks(&tasks, dir_path)) return false;

    bool found = false;
    da_foreach(Task, task, &tasks) {
        if (strcmp(task->id, huid) == 0) {
            if (path_only) {
                printf("%s/%s/TASK.md\n", rel_path, task->id);
            } else {
                print_task(rel_path, task);
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

uint32_t sv_key_hash(const String_View *a)
{
    return ht_djb2(a->data, a->count);
}

bool sv_key_eq(const String_View *a, const String_View *b)
{
    return sv_eq(*a, *b);
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

    const char *dir_path = find_tasks_database();
    if (!dir_path) return false;
    Tasks tasks = {0};
    if (!load_tasks(&tasks, dir_path)) return false;
    size_t total_count = 0;
    size_t untagged_count = 0;
    Hash_Table(String_View, size_t) tags_count = {
        .key_hash = sv_key_hash,
        .key_eq   = sv_key_eq,
    };
    for (size_t i = 0; i < tasks.count; ++i) {
        Task *task = &tasks.items[i];
        bool task_is_closed = strcmp(task->status, "CLOSED") == 0;
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
    ht_foreach(&tags_count) {
        if (max_width < tags_count.key.count) {
            max_width = tags_count.key.count;
        }
        da_append(&sorted_tags_count, ((Tag_Count) {
            .tag = tags_count.key,
            .count = *tags_count.value,
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
            printf("    %*s => %zu\n", (int)max_width, temp_sv_to_cstr(tag_count->tag), tag_count->count);
        }
    }

    return true;
}

static bool help_run(Command *self, const char *program_name, int argc, char **argv);

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
        .name = "help",
        .signature = "[OPTIONS]",
        .description = "Print this help message",
        .run = help_run,
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
            .rel_path = "../../../../../../rexim/Programming/tsoding/sofren/tasks",
        },
    };

    bool record = false;
    if (!record) {
        // Replay
        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            const char *dst_path    = cases[i].dst_path;
            const char *src_path    = cases[i].src_path;
            const char *description = cases[i].description;
            const char *rel_path    = cases[i].rel_path;
            printf("%s ...", description);
            fflush(stdout);
            const char *outcome = relative_path(dst_path, src_path);
            if (strcmp(outcome, rel_path) == 0) {
                printf(" OK\n");
            } else {
                printf(" FAILED\n");
                printf("  EXPECTED: %s\n", rel_path);
                printf("  ACTUAL:   %s\n", outcome);
                abort();
            }
        }
    } else {
        // Record
        printf("{\n");
        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            const char *dst_path    = cases[i].dst_path;
            const char *src_path    = cases[i].src_path;
            const char *description = cases[i].description;
            const char *rel_path    = relative_path(dst_path, src_path);
            printf("    {\n");
            printf("        .description = \"%s\",\n", description);
            printf("        .dst_path    = \"%s\",\n", dst_path);
            printf("        .src_path    = \"%s\",\n", src_path);
            printf("        .rel_path    = \"%s\",\n", rel_path);
            printf("    },\n");
        }
        printf("}\n");
    }
    return 0;

}
#endif // TASKS_TEST
