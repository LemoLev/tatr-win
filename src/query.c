#include "query.h"

static int not_end_of_query_token(int x)
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

bool task_matches_query(const Task *task, Query query, Stack *stack)
{
    stack->count = 0;
    da_foreach(Op, op, &query) {
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

void report_compile_query_error(String_View original_src, const String_View *src, const char *format, ...) NOB_PRINTF_FORMAT(3, 4);
void report_compile_query_error(String_View original_src, const String_View *src, const char *format, ...)
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

bool compile_query_expr(String_View original_src, String_View *src, Query *query);

bool compile_query_primary(String_View original_src, String_View *src, Query *query)
{
    *src = sv_trim_left(*src);
    if (src->count == 0) {
        report_compile_query_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
        return false;
    }
    if (*src->data == '.') {
        sv_chop_left(src, 1);
        String_View tag = sv_chop_while(src, not_end_of_query_token);
        da_append(query, ((Op) {
            .kind = OP_TAG,
            .tag = tag,
        }));
        return true;
    }
    if (*src->data == '(') {
        sv_chop_left(src, 1);
        if (!compile_query_expr(original_src, src, query)) return false;
        *src = sv_trim_left(*src);
        if (!sv_starts_with(*src, sv_from_cstr(")"))) {
            report_compile_query_error(original_src, src, "Expected `)`.");
            return false;
        }
        sv_chop_left(src, 1);
        return true;
    }
    String_View saved_src = *src;
    String_View key = sv_chop_while(src, not_end_of_query_token);
    if (sv_eq(key, sv_from_cstr("not"))) {
        if (!compile_query_primary(original_src, src, query)) return false;
        da_append(query, ((Op) { .kind = OP_NOT }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("any"))) {
        da_append(query, ((Op) { .kind = OP_ANY }));
        return true;
    }
    if (sv_eq(key, sv_from_cstr("tagged"))) {
        da_append(query, ((Op) { .kind = OP_TAGGED }));
        return true;
    }
    *src = saved_src;
    if (key.count == 0) {
        report_compile_query_error(original_src, src, "Expected `.`, `(`, `not`, `any`, or `tagged`.");
    } else {
        report_compile_query_error(original_src, src, "Unknown keyword `"SV_Fmt"`. Expected keywords `not`, `any`, or `tagged`.", SV_Arg(key));
    }
    return false;
}

bool compile_query_and(String_View original_src, String_View *src, Query *query)
{
    // <expr> [and <expr>]*
    if (!compile_query_primary(original_src, src, query)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_query_token);
        if (!sv_eq(key, sv_from_cstr("and"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_query_primary(original_src, src, query)) return false;
        da_append(query, ((Op) {.kind = OP_AND}));
    }
    return true;
}

bool compile_query_or(String_View original_src, String_View *src, Query *query)
{
    // <expr> [or <expr>]*
    if (!compile_query_and(original_src, src, query)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) {
            return true;
        }
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_query_token);
        if (!sv_eq(key, sv_from_cstr("or"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_query_and(original_src, src, query)) return false;
        da_append(query, ((Op) {.kind = OP_OR}));
    }
    return true;
}

bool compile_query_expr(String_View original_src, String_View *src, Query *query)
{
    if (!compile_query_or(original_src, src, query)) return false;
    return true;
}

bool compile_query(String_View original_src, String_View *src, Query *query)
{
    if (!compile_query_expr(original_src, src, query)) return false;
    *src = sv_trim_left(*src);
    if (src->count != 0) {
        report_compile_query_error(original_src, src, "Expected keywords `and`, or `or`");
        return false;
    }
    return true;
}
