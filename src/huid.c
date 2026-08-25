#include "huid.h"

bool chop_huid(String_View *content, String_View *huid)
{
    String_View copy = *content;
    for (size_t i = 0; copy.count > 0 && i < 8; ++i) {
        if (!isdigit(copy.data[0])) return false;
        sv_chop_left(&copy, 1);
    }
    if (!sv_eq(sv_chop_left(&copy, 1), sv_from_cstr("-"))) {
        return false;
    }
    for (size_t i = 0; copy.count > 0 && i < 6; ++i) {
        if (!isdigit(copy.data[0])) return false;
        sv_chop_left(&copy, 1);
    }
    huid->data  = content->data;
    huid->count = copy.data - content->data;
    *content = copy;
    return true;
}

bool is_valid_huid(const char *id)
{
    for (int i = 0; i < 8; ++i) if (!isdigit(*id++)) return false;
    if (*id++ != '-')                                return false;
    for (int i = 0; i < 6; ++i) if (!isdigit(*id++)) return false;
    if (*id++ != '\0')                               return false;
    return true;
}

char *temp_new_huid(void)
{
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = gmtime(&rawtime);
    char *id = temp_sprintf("%04d%02d%02d-%02d%02d%02d",
        timeinfo->tm_year+1900,
        timeinfo->tm_mon+1,
        timeinfo->tm_mday,
        timeinfo->tm_hour,
        timeinfo->tm_min,
        timeinfo->tm_sec);
    return id;
}
