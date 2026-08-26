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
} Op_Kind;

typedef struct {
    Op_Kind kind;
    String_View tag;
} Op;

void print_op(Op op);

typedef struct {
    Op *items;
    size_t count;
    size_t capacity;
} Query;

bool compile_query(String_View original_src, String_View *src, Query *query);

typedef struct {
    bool *items;
    size_t count;
    size_t capacity;
} Stack;

bool task_matches_query(const Task *task, Query query, Stack *stack);

#endif // QUERY_H_
