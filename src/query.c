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
#include "query.h"

void report_compile_query_diagnostic(String_View original_src, String_View src, const char *format, ...) NOB_PRINTF_FORMAT(3, 4);

int is_special(int x)
{
    if (x == '<') return true;
    if (x == '>') return true;
    if (x == '=') return true;
    if (x == '(') return true;
    if (x == ')') return true;
    if (x == '[') return true;
    if (x == ']') return true;
    if (x == ':') return true;
    if (x == '.') return true;
    if (x == '!') return true;
    return false;
}

int is_not_special_nor_space(int x)
{
    return !is_special(x) && !isspace(x);
}

String_View chop_next_query_token(String_View *src)
{
    *src = sv_trim_left(*src);

    if (src->count == 0) return *src;

    if (sv_starts_with(*src, SVLIT("<="))) return sv_chop_left(src, 2);
    if (sv_starts_with(*src, SVLIT(">="))) return sv_chop_left(src, 2);
    if (sv_starts_with(*src, SVLIT("=="))) return sv_chop_left(src, 2);
    if (sv_starts_with(*src, SVLIT("!="))) return sv_chop_left(src, 2);

    if (sv_starts_with(*src, SVLIT("<")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT(">")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT("=")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT("(")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT(")")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT("[")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT("]")))  return sv_chop_left(src, 1);

    if (sv_starts_with(*src, SVLIT(":")))  return sv_chop_left(src, 1);
    if (sv_starts_with(*src, SVLIT(".")))  return sv_chop_left(src, 1);

    // TASK(20260830-163947): TQL supports less tags than the spec allows to define
    return sv_chop_while(src, is_not_special_nor_space);
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
        report_compile_query_diagnostic(original_src, item->src, "ERROR: Expected %s but got %s", type_name(type), type_name(item->type));
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
        // TASK(20260829-225753): All comparison operators could work with booleans too
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

void report_compile_query_diagnostic(String_View original_src, String_View src, const char *format, ...)
{
    int cursor = sv_utf8_len((String_View) {
        .data = original_src.data,
        .count = src.data - original_src.data,
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
    String_View token = chop_next_query_token(src);

    // We used to use dot to refer to a tag before. We still accept it for backward compatibility.
    // But we may remove it at some point in the future.
    if (sv_eq(token, SVLIT(".")) || sv_eq(token, SVLIT(":"))) {
        if (sv_eq(token, SVLIT("."))) {
            report_compile_query_diagnostic(original_src, token, "WARNING: Using `.` to refer to tags is deprecated and will be removed in the future. Use `:` instead.");
        }
        String_View tag = chop_next_query_token(src);
        da_append(query, op_tag(token, tag));
        return true;
    }

    if (sv_eq(token, SVLIT("(")) || sv_eq(token, SVLIT("["))) {
        char start = *token.data;
        if (!compile_query_expr(original_src, src, query)) return false;
        token = chop_next_query_token(src);
        if (start == '(' && !sv_eq(token, SVLIT(")"))) {
            report_compile_query_diagnostic(original_src, token, "ERROR: Expected `)`.");
            return false;
        }
        if (start == '[' && !sv_eq(token, SVLIT("]"))) {
            report_compile_query_diagnostic(original_src, token, "ERROR: Expected `]`.");
            return false;
        }
        return true;
    }

    if (sv_eq(token, SVLIT("not"))) {
        if (!compile_query_primary(original_src, src, query)) return false;
        da_append(query, op_not(token));
        return true;
    }

    if (sv_eq(token, SVLIT("any"))) {
        da_append(query, op_any(token));
        return true;
    }

    if (sv_eq(token, SVLIT("tagged"))) {
        da_append(query, op_tagged(token));
        return true;
    }

    if (sv_eq(token, SVLIT("priority"))) {
        da_append(query, op_priority(token));
        return true;
    }

    size_t checkpoint = temp_save();
    char *endptr      = NULL;
    const char *nptr  = temp_sv_to_cstr(token);
    long integer      = strtol(nptr, &endptr, 10);
    bool ok           = nptr < endptr && *endptr == '\0';
    temp_rewind(checkpoint);
    if (ok) {
        da_append(query, op_integer(token, integer));
        return true;
    }

    fprintf(stderr, "What are primary expressions:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    :<tag>         - checks if task has a tag\n");
    fprintf(stderr, "    ( <expr> )     - grouped expression\n");
    fprintf(stderr, "    [ <expr> ]     - same as previous but for Bash users\n");
    fprintf(stderr, "    not <primary>  - negation of a primary expression\n");
    fprintf(stderr, "    any            - expression that always returns true\n");
    fprintf(stderr, "    tagged         - checks if a task is tagged\n");
    fprintf(stderr, "    priority       - priority of a task as an integer\n");
    fprintf(stderr, "    <number>       - signed integer\n");
    fprintf(stderr, "\n");
    if (token.count == 0) {
        report_compile_query_diagnostic(original_src, token, "ERROR: Primary expression is expected here.");
    } else {
        report_compile_query_diagnostic(original_src, token, "ERROR: Unexpected start of a primary expression `"SV_Fmt"`.", SV_Arg(token));
    }
    return false;
}

bool compile_query_compare(String_View original_src, String_View *src, Query *query)
{
    // <expr> [<compare> <expr>]*
    if (!compile_query_primary(original_src, src, query)) return false;
    for (;;) {
        String_View saved_src = *src;
        String_View token = chop_next_query_token(src);
        if (sv_eq(token, SVLIT("lt")) || sv_eq(token, SVLIT("<"))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_lt(token));
            continue;
        }
        if (sv_eq(token, SVLIT("le")) || sv_eq(token, SVLIT("<="))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_lte(token));
            continue;
        }
        if (sv_eq(token, SVLIT("gt")) || sv_eq(token, SVLIT(">"))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_gt(token));
            continue;
        }
        if (sv_eq(token, SVLIT("ge")) || sv_eq(token, SVLIT(">="))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_gte(token));
            continue;
        }
        if (sv_eq(token, SVLIT("eq")) || sv_eq(token, SVLIT("=")) || sv_eq(token, SVLIT("=="))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_eq(token));
            continue;
        }
        if (sv_eq(token, SVLIT("ne")) || sv_eq(token, SVLIT("!="))) {
            if (!compile_query_primary(original_src, src, query)) return false;
            da_append(query, op_neq(token));
            continue;
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
        String_View saved_src = *src;
        String_View token = chop_next_query_token(src);
        if (!sv_eq(token, sv_from_cstr("and"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_query_compare(original_src, src, query)) return false;
        da_append(query, op_and(token));
    }
    return true;
}

bool compile_query_or(String_View original_src, String_View *src, Query *query)
{
    // <expr> [or <expr>]*
    if (!compile_query_and(original_src, src, query)) return false;
    for (;;) {
        String_View saved_src = *src;
        String_View token = chop_next_query_token(src);
        if (!sv_eq(token, sv_from_cstr("or"))) {
            *src = saved_src;
            return true;
        }
        if (!compile_query_and(original_src, src, query)) return false;
        da_append(query, op_or(token));
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
    String_View end = chop_next_query_token(src);
    if (end.count != 0) {
        fprintf(stderr, "Supported infix operators:\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "    and  or                  - logical operators\n");
        fprintf(stderr, "    <  <=  >  >=  =  ==  !=  - comparison operators\n");
        fprintf(stderr, "    lt  le  gt  ge  eq  ne   - same as above but for Bash users\n");
        fprintf(stderr, "\n");
        report_compile_query_diagnostic(original_src, end, "ERROR: Unexpected infix operator `"SV_Fmt"`", SV_Arg(end));
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
