#include "task.h"

bool tags_contains(Tags tags, String_View tag)
{
    for (size_t i = 0; i < tags.count; ++i) {
        if (sv_eq(tags.items[i], tag)) return true;
    }
    return false;
}

void append_task_md_content(String_Builder *sb, Task task)
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

void print_tags(Tags tags)
{
    for (size_t i = 0; i < tags.count; ++i) {
        if (i > 0) printf(",");
        printf(SV_Fmt, SV_Arg(tags.items[i]));
    }
}

void print_task(const char *rel_path, Task *task)
{
    if (task->tags.count) {
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d, TAGS: ", rel_path, task->id, task->priority);
        print_tags(task->tags);
        printf("] "SV_Fmt"\n", SV_Arg(task->title));
    } else {
        printf("%s/%s/TASK.md:1: [PRIORITY: %-3d] "SV_Fmt"\n", rel_path, task->id, task->priority, SV_Arg(task->title));
    }
}

bool load_tasks(Tasks *tasks, const char *dir_path)
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
        if (!file_exists(task_md_path)) continue;
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

int task_compare_id(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return strcmp(ta->id, tb->id);
}

int task_compare_id_reverse(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return strcmp(tb->id, ta->id);
}

int task_compare_priority_reverse(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return tb->priority - ta->priority;
}

int task_compare_priority(const void *a, const void *b)
{
    const Task *ta = a;
    const Task *tb = b;
    return ta->priority - tb->priority;
}

Task_Compare task_sorter(bool by_id, bool ascending)
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

bool task_matches_tags(const Task *task, const char **tags, size_t tags_count)
{
    for (size_t j = 0; j < tags_count; ++j) {
        if (!tags_contains(task->tags, sv_from_cstr(tags[j]))) {
            return false;
        }
    }
    return true;
}
