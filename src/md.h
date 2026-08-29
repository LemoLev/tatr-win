#ifndef MD_H_
#define MD_H_

#include "ht.h"

typedef Ht(String_View, String_View) Properties;

void task_md_parse(char *task_md_content, String_View *title, Properties *ps);

#endif // MD_H_
