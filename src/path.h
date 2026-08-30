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
#ifndef PATH_H_
#define PATH_H_

#include "nob.h"

#ifdef _WIN32
    #define PATH_SEP "\\"
#else
    #define PATH_SEP "/"
#endif // _WIN32

typedef struct {
#ifdef _WIN32
    String_View disk;
#endif // _WIN32
    String_View *items;
    size_t count;
    size_t capacity;
} Path;

bool path_eq(Path a, Path b);
void path_normalize(Path *dst, Path src);
void path_render(String_Builder *sb, Path path);
char *path_render_cstr(String_Builder *sb, Path path);
void path_parse(Path *path, String_View sv);
void path_relative(Path *relative, Path current, Path target);

bool test_path_normalize(void);
bool test_path_parse_and_render(void);
bool test_path_relative(void);

#endif // PATH_H_
