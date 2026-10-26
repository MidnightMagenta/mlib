#include <assert.h>
#include <dynarr.h>
#include <dynarr_raw.h>
#include <ecode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ML_DYNARR_DEF_CAP 4

ml_dynarr ml_dynarr_new(size_t len, size_t obj_size, void (*dtor)(void *)) {
    ml_dynarr newarr;

    assert(obj_size != 0);

    newarr = malloc(sizeof(struct ml_dynarr));
    if (!newarr) { return NULL; }
    memset(newarr, 0, sizeof(struct ml_dynarr));
    newarr->elem_sz = obj_size;
    newarr->dtor    = dtor;

    if (ml_dynarr_resize(newarr, len ? len : ML_DYNARR_DEF_CAP)) {
        free(newarr);
        return NULL;
    }

    return newarr;
}

void ml_dynarr_free(ml_dynarr a) {
    assert(a);
    if (a->dtor) {
        for (size_t i = 0; i < a->len; i++) { a->dtor((char *) a->arr + i * a->elem_sz); }
    }
    free(a->arr);
    free(a);
}

ml_dynarr ml_dynarr_copy(ml_dynarr a) {
    assert(a);
    ml_dynarr newarr = ml_dynarr_new(a->capacity, a->elem_sz, a->dtor);
    if (!newarr) { return NULL; }
    memcpy(newarr->arr, a->arr, a->capacity * a->elem_sz);
    newarr->len = a->len;
    return newarr;
}

size_t ml_dynarr_size(ml_dynarr a) {
    assert(a);
    return a->len;
}

size_t ml_dynarr_capacity(ml_dynarr a) {
    assert(a);
    return a->capacity;
}

size_t ml_dynarr_elem_size(ml_dynarr a) {
    assert(a);
    return a->elem_sz;
}

void *ml_dynarr_data(ml_dynarr a) {
    assert(a);
    return a->arr;
}

int ml_dynarr_resize(ml_dynarr a, size_t new_cap) {
    void *tmp;
    assert(a);

    if (new_cap > SIZE_MAX / a->elem_sz) { return EC_VALUERRR; }

    if (new_cap < a->len) {
        if (a->dtor) {
            for (size_t i = new_cap; i < a->len; i++) { a->dtor((char *) a->arr + i * a->elem_sz); }
        }
        a->len = new_cap;
    }

    if (new_cap == 0) {
        free(a->arr);
        a->arr      = NULL;
        a->capacity = 0;
        return 0;
    }

    if (new_cap != a->capacity) {
        tmp = realloc(a->arr, new_cap * a->elem_sz);
        if (!tmp) { return new_cap > a->capacity ? EC_ALLOCERR : EC_NORMAL; }
        a->arr      = tmp;
        a->capacity = new_cap;
    }

    if (new_cap > a->len) { memset((char *) a->arr + a->len * a->elem_sz, 0, (new_cap - a->len) * a->elem_sz); }

    return EC_NORMAL;
}

int ml_dynarr_shrink_to_fit(ml_dynarr a) {
    assert(a);
    return ml_dynarr_resize(a, a->len);
}

int ml_dynarr_push_back(ml_dynarr a, void *obj) {
    assert(a);
    int res;
    if (a->len == a->capacity) {
        if ((res = ml_dynarr_resize(a, a->capacity ? a->capacity * 2 : ML_DYNARR_DEF_CAP))) { return res; }
    }
    memcpy((char *) a->arr + a->len * a->elem_sz, obj, a->elem_sz);
    a->len++;
    return EC_NORMAL;
}

int ml_dynarr_pop_back(ml_dynarr a) {
    assert(a);
    if (a->len == 0) { return EC_NORMAL; }
    a->len--;
    if (a->dtor) { a->dtor((char *) a->arr + a->len * a->elem_sz); }
    return EC_NORMAL;
}

void *ml_dynarr_get(ml_dynarr a, size_t idx) {
    assert(a);
    if (idx >= a->len) { return NULL; }
    return (char *) a->arr + idx * a->elem_sz;
}

void *ml_dynarr_put(ml_dynarr a, size_t idx, void *obj) {
    assert(a);
    if (idx >= a->len) { return NULL; }
    memcpy((char *) a->arr + idx * a->elem_sz, obj, a->elem_sz);
    return (char *) a->arr + idx * a->elem_sz;
}
