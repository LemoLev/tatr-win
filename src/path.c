#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void path_normalize(Path *dst, Path src)
{
    // TASK(20260825-162925): double check with python that path_normalize is implemented correctly
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

// TASK(20260825-195942): Move the relative path test into a separate unit
static void test_path(void)
{
    printf("------------------------------\n");

    // Normalization
    {
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
    }

    printf("------------------------------\n");

    // Rendering
    if (1) {
        String_View cases[] = {
            SVLIT_STATIC("/home"),
            SVLIT_STATIC("/home/test"),
            SVLIT_STATIC("home/test"),
            SVLIT_STATIC("/"),
            SVLIT_STATIC("home"),
        };

        Path path = {0};
        String_Builder sb = {0};
        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            if (i > 0) printf("\n");
            path_parse(&path, cases[i]);
            printf("original = \""SV_Fmt"\"\n", SV_Arg(cases[i]));
            printf("rendered = \"%s\"\n", path_render_cstr(&sb, path));
        }
    }

    printf("------------------------------\n");

    // Relative
    if (1) {
        Path relative     = {0};
        Path current      = {0};
        Path target       = {0};
        String_Builder sb = {0};

        static struct {
            String_View current;
            String_View target;
        } cases[] = {
            {
                .current = SVLIT_STATIC("/home/streamer/Programming/tsoding/tatr/thirdparty"),
                .target  = SVLIT_STATIC("/home/streamer/Programming/tsoding/tatr/task"),
            },
            {
                .current = SVLIT_STATIC("/home/streamer/Programming/tsoding/tatr/thirdparty"),
                .target  = SVLIT_STATIC("/poopoo/peepee/"),
            },
            {
                .current = SVLIT_STATIC("/poopoo/peepee/"),
                .target  = SVLIT_STATIC("/home/streamer/Programming/tsoding/tatr/thirdparty"),
            },
            {
                .current = SVLIT_STATIC("/"),
                .target  = SVLIT_STATIC("/"),
            },
            {
                .current = SVLIT_STATIC("/poopoo/peepee"),
                .target  = SVLIT_STATIC("/poopoo/peepee"),
            },
            {
                .current = SVLIT_STATIC("/poopoo"),
                .target  = SVLIT_STATIC("/poopoo/peepee/foo/bar"),
            },
            {
                .current = SVLIT_STATIC("/"),
                .target  = SVLIT_STATIC("/poopoo/peepee/foo/bar"),
            },
            {
                .current = SVLIT_STATIC("/poopoo/peepee/foo/bar"),
                .target  = SVLIT_STATIC("/"),
            },
            {
                .current = SVLIT_STATIC("/.poopoo/.peepee/.foo/.bar"),
                .target  = SVLIT_STATIC("/"),
            },
        };

        for (size_t i = 0; i < ARRAY_LEN(cases); ++i) {
            if (i > 0) printf("\n");
            path_parse(&current, cases[i].current);
            path_parse(&target,  cases[i].target);
            path_relative(&relative, current, target);

            printf("current  = %s\n", path_render_cstr(&sb, current));
            printf("target   = %s\n", path_render_cstr(&sb, target));
            printf("relative = %s\n", path_render_cstr(&sb, relative));
        }
    }

    printf("------------------------------\n");

    {
        Path path = {0};
        Path norm = {0};
        path_parse(&path, SVLIT("/hello///"));
        path_normalize(&norm, path);
        da_foreach(String_View, item, &norm) {
            printf("\"" SV_Fmt "\"\n", SV_Arg(*item));
        }
    }
}

bool path_eq(Path a, Path b)
{
    if (a.count != b.count) return false;
    for (size_t i = 0; i < a.count; ++i) {
        if (!sv_eq(a.items[i], b.items[i])) return false;
    }
    return true;
}
