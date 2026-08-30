// Copyright (C) 2026  Alexey Kutepov <reximkut@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see <https://www.gnu.org/licenses/>.
#include "huid.h"

bool is_alnum_or_dash(char x)
{
    return isalnum(x) || x == '-';
}

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
    if (sv_starts_with(copy, SVLIT("-"))) {
        while (copy.count > 0 && is_alnum_or_dash(copy.items[0])) {
            sv_chop_left(&copy, 1);
        }
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
    switch (*id++) {
    case '\0':  return true;  // Short
    case '-':   break;        // Extended
    default:    return false; // Invalid
    }
    while (*id) if (!is_alnum_or_dash(*id++))        return false;
    return true;
}

char *temp_new_huid(const char *suffix)
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
    if (suffix) {
        id = temp_sprintf("%s-%s", id, suffix);
    }
    return id;
}
