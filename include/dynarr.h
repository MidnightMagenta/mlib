#ifndef MLIB_DYNARR_H
#define MLIB_DYNARR_H

#include <stddef.h>

typedef struct ml_dynarr *ml_dynarr;

ml_dynarr ml_dynarr_new(size_t len, size_t obj_size, void (*dtor)(void *));
void      ml_dynarr_free(ml_dynarr a);
ml_dynarr ml_dynarr_copy(ml_dynarr a);

size_t ml_dynarr_size(ml_dynarr a);
size_t ml_dynarr_capacity(ml_dynarr a);
size_t ml_dynarr_elem_size(ml_dynarr a);
void  *ml_dynarr_data(ml_dynarr a);

int ml_dynarr_resize(ml_dynarr a, size_t new_size);
int ml_dynarr_shrink_to_fit(ml_dynarr a);

int ml_dynarr_push_back(ml_dynarr a, void *obj);
int ml_dynarr_pop_back(ml_dynarr a);

void *ml_dynarr_get(ml_dynarr a, size_t idx);
void *ml_dynarr_put(ml_dynarr a, size_t idx, void *obj);

#endif
