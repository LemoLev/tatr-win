#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void path_normalize(Path *dst, Path src)
{
    // TASK(20260825-162925): double check with python that path_normalize is implemented correctly
    dst->count = 0;
    for (size_t i = 0; i < src.count; ++i) {
        String_View sv = src.items[i];
        if (sv_eq(sv, sv_from_cstr("")) && i > 0) continue;
        if (sv_eq(sv, sv_from_cstr("."))) continue;
        if (sv_eq(sv, sv_from_cstr(".."))) {
            if (dst->count > 0) UNUSED(da_pop(dst));
            continue;
        }
        da_append(dst, sv);
    }
}

bool test_path_normalize(void)
{
    bool result = true;
    Path src_path = {0};
    Path dst_path = {0};
    String_Builder sb_path = {0};

    printf("%s ...", __func__);
    fflush(stdout);

    static struct {
        const char *input;
        const char *expected_output;
    } cases[] = {
        {
            .input           = "/home//rexim/./Programming/tsoding/../probe/tatr/tasks/20260321-181305",
            .expected_output = "/home/rexim/Programming/probe/tatr/tasks/20260321-181305",
        },
        {
            .input           = "/hello///",
            .expected_output = "/hello",
        }
    };

    for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
        path_parse(&src_path, sv_from_cstr(cases[i].input));
        path_normalize(&dst_path, src_path);
        const char *actual_output = path_render_cstr(&sb_path, dst_path);

        if (strcmp(cases[i].expected_output, actual_output) != 0) {
            printf(" FAILED\n");
            printf("  EXPECTED: %s\n", cases[i].expected_output);
            printf("  ACTUAL:   %s\n", actual_output);
            return_defer(false);
        }
    }
    printf(" OK\n");
defer:
    free(src_path.items);
    free(dst_path.items);
    free(sb_path.items);
    return result;
}

void path_render(String_Builder *sb, Path path)
{
    sb->count = 0;
#ifdef _WIN32
    sb_append_sv(sb, path.disk);
#endif // _WIN32
    for (size_t i = 0; i < path.count; ++i) {
        sb_append_sv(sb, path.items[i]);
        if ((i == 0 && sv_eq(path.items[i], SVLIT(""))) || i + 1 < path.count) {
            sb_append_cstr(sb, PATH_SEP);
        }
    }
}

char *path_render_cstr(String_Builder *sb, Path path)
{
    path_render(sb, path);
    sb_append_null(sb);
    return sb->items;
}

void path_parse(Path *path, String_View sv)
{
    path->count = 0;
#ifdef _WIN32
    TODO("path_parse on Windows");
#endif // _WIN32
    while (sv.count > 0) {
        String_View c = sv_chop_by_delim(&sv, '/');
        da_append(path, c);
    }
}

bool test_path_parse_and_render(void)
{
    bool result = true;
    Path path = {0};
    String_Builder sb_path = {0};

    static String_View cases[] = {
        SVLIT_STATIC("/home"),
        SVLIT_STATIC("/home/test"),
        SVLIT_STATIC("home/test"),
        SVLIT_STATIC("/"),
        SVLIT_STATIC("home"),
    };

    printf("%s ...", __func__);
    fflush(stdout);

    for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
        path_parse(&path, cases[i]);
        path_render(&sb_path, path);
        String_View output = sb_to_sv(sb_path);
        if (!sv_eq(cases[i], output)) {
            printf(" FAILED\n");
            printf("  EXPECTED: "SV_Fmt"\n", SV_Arg(cases[i]));
            printf("  ACTUAL:   "SV_Fmt"\n", SV_Arg(output));
            return_defer(false);
        }
    }
    printf(" OK\n");
defer:
    free(path.items);
    free(sb_path.items);
    return result;
}

void path_relative(Path *rel, Path src, Path dst)
{
    // Both paths are expected to be absolute and normalized
    assert(sv_eq(da_first(&src), sv_from_cstr("")));
    assert(sv_eq(da_first(&dst), sv_from_cstr("")));

    rel->count = 0;

    size_t i = 0;
    while (i < src.count && i < dst.count && sv_eq(src.items[i], dst.items[i])) {
        i += 1;
    }

    for (size_t j = i; j < src.count; ++j) {
        da_append(rel, sv_from_cstr(".."));
    }

    if (rel->count == 0) {
        da_append(rel, sv_from_cstr("."));
    }

    for (size_t j = i; j < dst.count; ++j) {
        da_append(rel, dst.items[j]);
    }
}

bool test_path_relative(void)
{
    bool result = true;

    Path dst_path          = {0};
    Path src_path          = {0};
    Path rel_path_expected = {0};
    Path rel_path_actual   = {0};
    String_Builder sb_path = {0};

    static struct {
        const char *dst_path;
        const char *src_path;
        const char *rel_path;
    } cases[] = {
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren",
            .rel_path = "./tasks",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks/",
            .src_path = "/home/rexim/Programming/tsoding/sofren",
            .rel_path = "./tasks",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = ".",
        },
        {
            .dst_path = "/",
            .src_path = "/",
            .rel_path = ".",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks/20250831-161356/",
            .rel_path = "..",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/src",
            .rel_path = "../tasks",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/src/game/",
            .rel_path = "../../tasks",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home_/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = "../../../../../../home/rexim/Programming/tsoding/sofren/tasks",
        },
        {
            .dst_path = "/home_/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = "../../../../../../home_/rexim/Programming/tsoding/sofren/tasks",
        },
        {
            .dst_path = "/",
            .src_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .rel_path = "../../../../../..",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/sofren/tasks",
            .src_path = "/",
            .rel_path = "./home/rexim/Programming/tsoding/sofren/tasks",
        },
        {
            .dst_path = "/home/rexim/Programming/tsoding/tatr/tasks",
            .src_path = "/home/rexim/Programming/tsoding/tatr/thirdparty",
            .rel_path = "../tasks",
        },
    };

    printf("%s ...", __func__);
    fflush(stdout);

    for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
        path_parse(&dst_path,          sv_from_cstr(cases[i].dst_path));
        path_parse(&src_path,          sv_from_cstr(cases[i].src_path));
        path_parse(&rel_path_expected, sv_from_cstr(cases[i].rel_path));
        path_relative(&rel_path_actual, src_path, dst_path);
        if (!path_eq(rel_path_actual, rel_path_expected)) {
            printf(" FAILED\n");
            printf("  EXPECTED: %s\n", path_render_cstr(&sb_path, rel_path_expected));
            printf("  ACTUAL:   %s\n", path_render_cstr(&sb_path, rel_path_actual));
            return_defer(false);
        }
    }

    printf(" OK\n");
defer:
    free(dst_path.items);
    free(src_path.items);
    free(rel_path_expected.items);
    free(rel_path_actual.items);
    free(sb_path.items);
    return result;
}

bool path_eq(Path a, Path b)
{
    if (a.count != b.count) return false;
    for (size_t i = 0; i < a.count; ++i) {
        if (!sv_eq(a.items[i], b.items[i])) return false;
    }
    return true;
}
