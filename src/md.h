#ifndef MD_H_
#define MD_H_

typedef struct {
    char *file;
    char *source;
    size_t cur, bol, line;
} Md;

void md_init(Md *md, char *file, char *source);
char md_char(Md *md);
void md_trim_spaces(Md *md);
bool task_md_extract_title(Md *md, String_View *title);
bool task_md_extract_field(Md *md, char *field, String_View *value);

#endif // MD_H_
