#ifndef FILTER_H_
#define FILTER_H_

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
} Filter;

bool compile_filter(String_View original_src, String_View *src, Filter *filter);

typedef struct {
    bool *items;
    size_t count;
    size_t capacity;
} Stack;

bool task_matches_filter(const Task *task, Filter filter, Stack *stack);

#endif // FILTER_H_
