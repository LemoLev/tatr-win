#include "md.h"

static inline bool isspace_except_newline(char x)
{
    return isspace(x) && x != '\n' && x != '\r';
}

void md_init(Md *md, char *file, char *source)
{
    memset(md, 0, sizeof(*md));
    md->file = file;
    md->source = source;
    md->line = 1;
}

size_t md_col(Md *md)
{
    return md->cur - md->bol + 1;
}

char *md_cstr(Md *md)
{
    return &md->source[md->cur];
}

char md_char(Md *md)
{
    return md->source[md->cur];
}

bool md_end(Md *md)
{
    return md_char(md) == '\0';
}

char md_next_char(Md *md)
{
    if (md_end(md)) return '\0';
    char x = md->source[md->cur++];
    if (x == '\n') {
        md->bol = md->cur;
        md->line += 1;
    }
    return x;
}

bool md_expect_char(Md *md, char x)
{
    if (md_char(md) != x) return false;
    md_next_char(md);
    return true;
}

bool md_expect_str(Md *md, char *str)
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

void md_trim_spaces(Md *md)
{
    while (isspace(md_char(md))) {
        md_next_char(md);
    }
}

void md_trim_spaces_except_newline(Md *md)
{
    while (isspace_except_newline(md_char(md))) {
        md_next_char(md);
    }
}

String_View md_chop_until_newline(Md *md)
{
    char *start = md_cstr(md);
    while (!md_end(md) && md_char(md) != '\n') {
        md_next_char(md);
    }
    return sv_from_parts(start, md_cstr(md) - start);
}

bool task_md_extract_title(Md *md, String_View *title)
{
    md_trim_spaces(md);
    if (!md_expect_char(md, '#')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected '#' as a start of a title, but got %c\n", md->file, md->line, md_col(md), md_char(md));
        return false;
    }
    md_trim_spaces_except_newline(md);
    *title = md_chop_until_newline(md);
    return true;
}

bool task_md_extract_field(Md *md, char *field, String_View *value)
{
    md_trim_spaces(md);
    if (!md_expect_char(md, '-')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected '-' as a start of a TASK.md field '%s', but got %c\n", md->file, md->line, md_col(md), field, md_char(md));
        return false;
    }
    md_trim_spaces_except_newline(md);
    if (!md_expect_str(md, field)) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected a TASK.md field name '%s'\n", md->file, md->line, md_col(md), field);
        return false;
    }
    md_trim_spaces_except_newline(md);
    if (!md_expect_char(md, ':')) {
        fprintf(stderr, "%s:%zu:%zu: ERROR: expected a field separator ':'\n", md->file, md->line, md_col(md));
        return false;
    }
    md_trim_spaces_except_newline(md);
    *value = md_chop_until_newline(md);
    return true;
}
