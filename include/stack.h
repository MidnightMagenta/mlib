#ifndef LIB_STACK_H
#define LIB_STACK_H

#include <stddef.h>

typedef struct stack *stack_t;

stack_t stack_create(size_t elem_sz, void (*dtor)(void *elem), size_t initial_capacity);
void    stack_free(stack_t s);
int     stack_push(stack_t s, const void *elem);
int     stack_pop(stack_t s, void *out);
void   *stack_peek(const stack_t s);

#endif
