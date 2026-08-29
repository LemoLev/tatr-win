#include "query.h"

void report_compile_query_diagnostic(String_View original_src, const String_View *src, const char *format, ...) NOB_PRINTF_FORMAT(3, 4);

static int not_end_of_query_token(int x)
{
    return x != ' ' && x != ')' && x != '(' && x != ']' && x != '[';
}

void print_op(Op op)
{
    switch (op.kind) {
    case OP_ANY:      printf("OP_ANY\n");                             break;
    case OP_TAG:      printf("OP_TAG "SV_Fmt"\n", SV_Arg(op.as.tag)); break;
    case OP_NOT:      printf("OP_NOT\n");                             break;
    case OP_OR:       printf("OP_OR\n");                              break;
    case OP_AND:      printf("OP_AND\n");                             break;
    case OP_TAGGED:   printf("OP_TAGGED\n");                          break;
    case OP_PRIORITY: printf("OP_PRIORITY\n");                        break;
    case OP_INTEGER:  printf("OP_INTEGER %ld\n", op.as.integer);      break;
    case OP_LT:       printf("OP_LT\n");                              break;
    case OP_GT:       printf("OP_GT\n");                              break;
    case OP_LTE:      printf("OP_LTE\n");                             break;
    case OP_GTE:      printf("OP_GTE\n");                             break;
    case OP_EQ:       printf("OP_EQ\n");                              break;
    case OP_NEQ:      printf("OP_NEQ\n");                             break;
    default: UNREACHABLE("Op_Kind");
    }
}

bool pop_type(String_View original_src, Stack *stack, Type type, Stack_Item *item)
{
    *item = da_pop(stack);
    if (item->type != type) {
        report_compile_query_diagnostic(original_src, &item->src, "ERROR: Expected %s but got %s", type_name(type), type_name(item->type));
        return false;
    }
    return true;
}

Task_Match_Result task_matches_query(String_View original_src, const Task *task, Query query, Stack *stack)
{
    stack->count = 0;
    da_foreach(Op, op, &query) {
        switch (op->kind) {
        case OP_ANY: {
            da_append(stack, stack_bool(op->src, true));
        } break;
        case OP_TAG: {
            bool result = tags_contains(task->tags, op->as.tag);
            da_append(stack, stack_bool(op->src, result));
        } break;
        case OP_NOT: {
            Stack_Item item = {0};
            if (!pop_type(original_src, stack, TYPE_BOOLEAN, &item)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, !item.as.boolean));
        } break;
        case OP_OR: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_BOOLEAN, &a)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_BOOLEAN, &b)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.boolean || b.as.boolean));
        } break;
        case OP_AND: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_BOOLEAN, &a)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_BOOLEAN, &b)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.boolean && b.as.boolean));
        } break;
        case OP_TAGGED: {
            da_append(stack, stack_bool(op->src, task->tags.count > 0));
        } break;
        case OP_PRIORITY: {
            da_append(stack, stack_int(op->src, task->priority));
        } break;
        case OP_INTEGER: {
            da_append(stack, stack_int(op->src, op->as.integer));
        } break;
        case OP_LT: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer < b.as.integer));
        } break;
        case OP_GT: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer > b.as.integer));
        } break;
        case OP_LTE: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer <= b.as.integer));
        } break;
        case OP_GTE: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer >= b.as.integer));
        } break;
        // TASK(20260829-225753): `OP_EQ` and `OP_NEQ` could work with booleans too
        case OP_EQ: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer == b.as.integer));
        } break;
        case OP_NEQ: {
            Stack_Item a = {0};
            Stack_Item b = {0};
            if (!pop_type(original_src, stack, TYPE_INTEGER, &b)) return TMR_ERROR;
            if (!pop_type(original_src, stack, TYPE_INTEGER, &a)) return TMR_ERROR;
            da_append(stack, stack_bool(op->src, a.as.integer != b.as.integer));
        } break;
        default: UNREACHABLE("Op_Kind");
        }
    }
    Stack_Item result = {0};
    if (!pop_type(original_src, stack, TYPE_BOOLEAN, &result)) {
        return TMR_ERROR;
    }
    if (result.as.boolean) {
        return TMR_MATCHED;
    }
    return TMR_MISMATCHED;
}

void report_compile_query_diagnostic(String_View original_src, const String_View *src, const char *format, ...)
{
    int cursor = sv_utf8_len((String_View) {
        .data = original_src.data,
        .count = src->data - original_src.data,
    }, NULL) + 1;
    fprintf(stderr, SV_Fmt"\n", SV_Arg(original_src));
    fprintf(stderr, "%*s\n", cursor, "^");
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
        report_compile_query_diagnostic(original_src, src, "ERROR: Expected `:`, `(`, `not`, `any`, or `tagged`.");
        return false;
    }
    // We used to use dot to refer to a tag before. We still accept it for backward compatibility.
    // But we may remove it at some point in the future.
    if (*src->data == '.' || *src->data == ':') {
        if (*src->data == '.') {
            report_compile_query_diagnostic(original_src, src, "WARNING: Using `.` to refer to tags is deprecated and will be removed in the future. Use `:` instead.");
        }
        sv_chop_left(src, 1);
        String_View tag = sv_chop_while(src, not_end_of_query_token);
        da_append(query, op_tag(*src, tag));
        return true;
    }
    if (*src->data == '(' || *src->data == '[') {
        char start = *src->data;
        sv_chop_left(src, 1);
        if (!compile_query_expr(original_src, src, query)) return false;
        *src = sv_trim_left(*src);
        if (start == '(' && !sv_starts_with(*src, sv_from_cstr(")"))) {
            report_compile_query_diagnostic(original_src, src, "ERROR: Expected `)`.");
            return false;
        }
        if (start == '[' && !sv_starts_with(*src, sv_from_cstr("]"))) {
            report_compile_query_diagnostic(original_src, src, "ERROR: Expected `]`.");
            return false;
        }
        sv_chop_left(src, 1);
        return true;
    }
    String_View saved_src = *src;
    String_View key = sv_chop_while(src, not_end_of_query_token);
    if (sv_eq(key, SVLIT("not"))) {
        if (!compile_query_primary(original_src, src, query)) return false;
        da_append(query, op_not(saved_src));
        return true;
    }
    if (sv_eq(key, SVLIT("any"))) {
        da_append(query, op_any(saved_src));
        return true;
    }
    if (sv_eq(key, SVLIT("tagged"))) {
        da_append(query, op_tagged(saved_src));
        return true;
    }
    if (sv_eq(key, SVLIT("priority"))) {
        da_append(query, op_priority(saved_src));
        return true;
    }

    size_t checkpoint = temp_save();
    char *endptr = NULL;
    const char *nptr = temp_sv_to_cstr(key);
    long integer = strtol(nptr, &endptr, 10);
    if (nptr < endptr && *endptr == '\0') {
        da_append(query, op_integer(saved_src, integer));
        temp_rewind(checkpoint);
        return true;
    }
    temp_rewind(checkpoint);

    *src = saved_src;
    if (key.count == 0) {
        report_compile_query_diagnostic(original_src, src, "ERROR: Expected `:`, `(`, `not`, `any`, `tagged`, or `priority`.");
    } else {
        report_compile_query_diagnostic(original_src, src, "ERROR: Unknown keyword `"SV_Fmt"`. Expected keywords `not`, `any`, `tagged`, or `priority`.", SV_Arg(key));
    }
    return false;
}

bool compile_query_compare(String_View original_src, String_View *src, Query *query)
{
    // <expr> [<compare> <expr>]*
    if (!compile_query_primary(original_src, src, query)) return false;
    for (;;) {
        *src = sv_trim_left(*src);
        if (src->count == 0) break;
        String_View saved_src = *src;
        String_View key = sv_chop_while(src, not_end_of_query_token);
        if (sv_eq(key, sv_from_cstr("below"))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_lt(*src));
            continue;
        }
        if (sv_eq(key, sv_from_cstr("above"))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_gt(*src));
            continue;
        }
        if (sv_eq(key, sv_from_cstr("equal"))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_eq(*src));
            continue;
        }
        if (sv_eq(key, sv_from_cstr("not"))) {
            *src = sv_trim_left(*src);
            saved_src = *src;
            key = sv_chop_while(src, not_end_of_query_token);
            if (sv_eq(key, SVLIT("above"))) {
                if (!compile_query_primary(original_src, src, query)) return false;
                da_append(query, op_lte(*src));
                continue;
            }
            if (sv_eq(key, SVLIT("below"))) {
                if (!compile_query_primary(original_src, src, query)) return false;
                da_append(query, op_gte(*src));
                continue;
            }
            if (sv_eq(key, SVLIT("equal"))) {
                if (!compile_query_primary(original_src, src, query)) return false;
                da_append(query, op_neq(*src));
                continue;
            }
            report_compile_query_diagnostic(original_src, &saved_src, "ERROR: Expected keywords `above`, `below`, or `equal`");
            return false;
        }
        *src = saved_src;
        break;
    }
    return true;
}

bool compile_query_and(String_View original_src, String_View *src, Query *query)
{
    // <expr> [and <expr>]*
    if (!compile_query_compare(original_src, src, query)) return false;
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
        if (!compile_query_compare(original_src, src, query)) return false;
        da_append(query, op_and(*src));
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
        da_append(query, op_or(*src));
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
        // TASK(20260829-221717): we actually expect much more infix keywords when the expression is not fully parsed
        report_compile_query_diagnostic(original_src, src, "ERROR: Expected keywords `and`, or `or`");
        return false;
    }
    return true;
}

Stack_Item stack_bool(String_View src, bool value)
{
    return (Stack_Item) {
        .type = TYPE_BOOLEAN,
        .src = src,
        .as = { .boolean = value },
    };
}

Stack_Item stack_int(String_View src, int value)
{
    return (Stack_Item) {
        .type = TYPE_INTEGER,
        .src = src,
        .as = { .integer = value },
    };
}

const char *type_name(Type type)
{
    switch (type) {
    case TYPE_BOOLEAN: return "boolean";
    case TYPE_INTEGER: return "integer";
    case __type_count:
    default:
        UNREACHABLE("type_name");
    }
    return NULL;
}

Op op(Op_Kind kind, String_View src)
{
    return (Op) { .kind = kind, .src = src };
}

Op op_set_tag(Op op, String_View tag)
{
    op.as.tag = tag;
    return op;
}

Op op_set_integer(Op op, long integer)
{
    op.as.integer = integer;
    return op;
}
