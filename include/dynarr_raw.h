#ifndef MLIB_DYNARR_RAW_H
#define MLIB_DYNARR_RAW_H

#include <stddef.h>

struct ml_dynarr {
    void  *arr;
    size_t len;
    size_t capacity;
    size_t elem_sz;
    void   (*dtor)(void *);
};

#endif
