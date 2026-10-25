#include <ecode.h>
#include <lib/stack.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STACK_INITIAL_CAP 64

struct stack {
    void  *s;
    size_t elem_sz;
    size_t len;
    size_t cap;
    void   (*dtor)(void *elem);
};

stack_t stack_create(size_t elem_sz, void (*dtor)(void *elem), size_t initial_capacity) {
    stack_t s;
    if (initial_capacity == 0) { initial_capacity = STACK_INITIAL_CAP; }

    s = malloc(sizeof(*s));
    if (!s) { return NULL; }

    s->elem_sz = elem_sz;
    s->len     = 0;
    s->dtor    = dtor;
    s->cap     = initial_capacity;
    s->s       = aligned_alloc(8, s->cap * s->elem_sz);

    return s;
}

void stack_free(stack_t s) {
    if (s->dtor) {
        for (size_t i = 0; i < s->len; i++) { s->dtor((char *) s->s + i * s->elem_sz); }
    }
    free(s->s);
    free(s);
}

int stack_push(stack_t s, const void *elem) {
    if (s->len == s->cap) {
        size_t newcap = s->cap ? s->cap * 2 : STACK_INITIAL_CAP;
        if (newcap > SIZE_MAX / s->elem_sz) { return EC_ALLOCERR; }
        void *tmp = realloc(s->s, newcap * s->elem_sz);
        if (!tmp) { return EC_ALLOCERR; }
        s->s   = tmp;
        s->cap = newcap;
    }
    memcpy((char *) s->s + s->len * s->elem_sz, elem, s->elem_sz);
    s->len++;
    return EC_NORMAL;
}

int stack_pop(stack_t s, void *out) {
    if (s->len == 0) { return EC_INVAL; }
    s->len--;
    if (out) { memcpy(out, (char *) s->s + s->len * s->elem_sz, s->elem_sz); }
    return EC_NORMAL;
}

void *stack_peek(const stack_t s) {
    return s->len ? (char *) s->s + (s->len - 1) * s->elem_sz : NULL;
}
