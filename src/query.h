#ifndef QUERY_H_
#define QUERY_H_

#include "task.h"

typedef enum {
    OP_ANY,
    OP_TAG,
    OP_NOT,
    OP_OR,
    OP_AND,
    OP_TAGGED,
    OP_PRIORITY,
    OP_INTEGER,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE,
    OP_EQ,
    OP_NEQ,
} Op_Kind;

typedef struct {
    Op_Kind kind_;
    union {
        String_View tag;
        long integer;
    } as;
    String_View src;
} Op;

void print_op(Op op);

typedef struct {
    Op *items;
    size_t count;
    size_t capacity;
} Query;

bool compile_query(String_View original_src, String_View *src, Query *query);

typedef enum {
    TYPE_BOOL,
    TYPE_INT,
    __type_count,
} Type;

const char *type_name(Type type);

typedef struct {
    Type type;
    String_View src;
    union {
        static_assert(__type_count == 2, "Amount of stack item tags has changed");
        bool boolean;
        long integer;
    } as;
} Stack_Item;

static_assert(__type_count == 2, "Amount of stack item tags has changed");
Stack_Item stack_bool(String_View src, bool value);
Stack_Item stack_int(String_View src, int value);

typedef struct {
    Stack_Item *items;
    size_t count;
    size_t capacity;
} Stack;

typedef enum {
    TMR_MATCHED,
    TMR_MISMATCHED,
    TMR_ERROR,
} Task_Match_Result;

Task_Match_Result task_matches_query(String_View original_src, const Task *task, Query query, Stack *stack);

#endif // QUERY_H_
