#include "filter.h"

static int not_end_of_filter_token(int x)
{
    return x != ' ' && x != ')' && x != '(';
}

void print_op(Op op)
{
    switch (op.kind) {
    case OP_ANY:      printf("OP_ANY\n");                          break;
    case OP_TAG:      printf("OP_TAG "SV_Fmt"\n", SV_Arg(op.tag)); break;
    case OP_NOT:      printf("OP_NOT\n");                          break;
    case OP_OR:       printf("OP_OR\n");                           break;
    case OP_AND:      printf("OP_AND\n");                          break;
    case OP_TAGGED:   printf("OP_TAGGED\n");                       break;
    default: UNREACHABLE("Op_Kind");
    }
}

bool task_matches_filter(const Task *task, Filter filter, Stack *stack)
{
    stack->count = 0;
    da_foreach(Op, op, &filter) {
        switch (op->kind) {
        case OP_ANY: {
            da_append(stack, true);
        } break;
        case OP_TAG: {
            bool result = tags_contains(task->tags, op->tag);
            da_append(stack, result);
        } break;
        case OP_NOT: {
            da_last(stack) = !da_last(stack);
        } break;
        case OP_OR: {
            bool a = da_pop(stack);
            bool b = da_pop(stack);
            da_append(stack, a || b);
        } break;
        case OP_AND: {
            bool a = da_pop(stack);
            bool b = da_pop(stack);
            da_append(stack, a && b);
        } break;
        case OP_TAGGED: {
            da_append(stack, task->tags.count > 0);
        } break;
        default: UNREACHABLE("Op_Kind");
        }
    }
    return da_first(stack);
}

void report_compile_filter_error(String_View original_src, const String_View *src, const char *format, ...) NOB_PRINTF_FORMAT(3, 4);
void report_compile_filter_error(String_View original_src, const String_View *src, const char *format, ...)
{
    int cursor = sv_utf8_len((String_View) {
        .data = original_src.data,
        .count = src->data - original_src.data,
    }, NULL) + 1;
    fprintf(stderr, SV_Fmt"\n", SV_Arg(original_src));
    fprintf(stderr, "%*s\n", cursor, "^");
    fprintf(stderr, "ERROR: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool compile_filter_expr(String_View original_src, String_View *src, Filter *filter);

bool compile_filter_primary(String_View original_src, String_View *src, Filter *filter)
{
    *src = sv_trim_left(*src);
    if (src->count == 0) {
        report_compile_filter_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
        return false;
    }
    if (*src->data == '.') {
        sv_chop_left(src, 1);
        String_View tag = sv_chop_while(src, not_end_of_filter_token);
        da_append(filter, ((Op) {
            .kind = OP_TAG,
            .tag = tag,
        }));
        return true;
    }
    if (*src->data == '(') {
        sv_chop_left(src, 1);
        if (!compile_filter_expr(original_src, src, filter)) return false;
        *src = sv_trim_left(*src);
        if (!sv_starts_with(*src, sv_from_cstr(")"))) {
            report_compile_filter_error(original_src, src, "Expected `)`.");
            return false;
        }
        sv_chop_left(src, 1);
        return true;
    }
    String_View saved_src = *src;
    String_View key = sv_chop_while(src, not_end_of_filter_token);
    if (sv_eq(key, sv_from_cstr("not"))) {
        if (!compile_filter_primary(original_src, src, filter)) return false;
        da_append(filter, ((Op) { .kind = OP_NOT }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("any"))) {
        da_append(filter, ((Op) { .kind = OP_ANY }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("tagged"))) {
        da_append(filter, ((Op) { .kind = OP_TAGGED }));
        return true;
    }
    *src = saved_src;
    if (key.count == 0) {
        report_compile_filter_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
    } else {
        report_compile_filter_error(original_src, src, "Unknown keyword `"SV_Fmt"`. Expected keywords `not`, `any`, or `tagged`.", SV_Arg(key));
    }
    return false;
}

bool compile_filter_and(String_View original_src, String_View *src, Filter *filter)
{
    // <expr> [and <expr>]*
    if (!compile_filter_primary(original_src, src, filter)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_filter_token);
        if (!sv_eq(key, sv_from_cstr("and"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_filter_primary(original_src, src, filter)) return false;
        da_append(filter, ((Op) {.kind = OP_AND}));
    }
    return true;
}

bool compile_filter_or(String_View original_src, String_View *src, Filter *filter)
{
    // <expr> [or <expr>]*
    if (!compile_filter_and(original_src, src, filter)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_filter_token);
        if (!sv_eq(key, sv_from_cstr("or"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_filter_and(original_src, src, filter)) return false;
        da_append(filter, ((Op) {.kind = OP_OR}));
    }
    return true;
}

bool compile_filter_expr(String_View original_src, String_View *src, Filter *filter)
{
    if (!compile_filter_or(original_src, src, filter)) return false;
    return true;
}

bool compile_filter(String_View original_src, String_View *src, Filter *filter)
{
    if (!compile_filter_expr(original_src, src, filter)) return false;
    *src = sv_trim_left(*src);
    if (src->count != 0) {
        report_compile_filter_error(original_src, src, "Expected keywords `and`, or `or`");
        return false;
    }
    return true;
}
