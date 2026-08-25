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

#endif // PATH_H_
