#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define NOB_IMPLEMENTATION
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

void path_normalize(Path *dst, Path src) {
    // TODO: double check with python path normalization implementation
    for (size_t i = 0; i < src.count; ++i) {
        String_View sv = src.items[i];
        if (sv_eq(sv, sv_from_cstr("")) && i > 0) continue;
        if (sv_eq(sv, sv_from_cstr("."))) continue;
        if (sv_eq(sv, sv_from_cstr(".."))) {
            if (dst->count > 0) da_pop(dst);
            continue;
        }
        da_append(dst, sv);
    }
}

void path_render(String_Builder *sb, Path path) {
#ifdef _WIN32
    sb_append_sv(sb, path.disk);
#endif // _WIN32
    for (size_t i = 0; i < path.count; ++i) {
        if (i > 0) sb_append_cstr(sb, PATH_SEP);
        sb_append_sv(sb, path.items[i]);
    }
}

void path_parse(Path *path, String_View sv) {
#ifdef _WIN32
    TODO("path_parse on Windows");
#endif // _WIN32
    while (sv.count > 0) {
        String_View c = sv_chop_by_delim(&sv, '/');
        da_append(path, c);
    }
}

void path_relative(Path *relative, Path current, Path target)
{
    // Both paths are expected to be absolute and normalized
    assert(sv_eq(da_first(&current), sv_from_cstr("")));
    assert(sv_eq(da_first(&target), sv_from_cstr("")));

    size_t i = 0;
    while (i < current.count && i < target.count && sv_eq(current.items[i], target.items[i])) {
        i += 1;
    }

    for (size_t j = i; j < current.count; ++j) {
        da_append(relative, sv_from_cstr(".."));
    }

    for (size_t j = i; j < target.count; ++j) {
        da_append(relative, target.items[i]);
    }
}

int main() {
    const char *s = "/home//rexim/./Programming/tsoding/../probe/tatr/tasks/20260321-181305";
    Path src_path = {0};
    Path dst_path = {0};
    String_Builder sb = {0};

    path_parse(&src_path, sv_from_cstr(s));
    path_normalize(&dst_path, src_path);
    path_render(&sb, dst_path);
    sb_append_null(&sb);

    printf("%s\n", s);
    printf("%s\n", sb.items);
    return 0;
}
