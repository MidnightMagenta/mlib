#ifndef MLIB_HASHMAP_H
#define MLIB_HASHMAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct ml_hashmap *ml_hashmap;

typedef size_t (*hm_hash_fn)(const void *key);
typedef int    (*hm_eq_fn)(const void *a, const void *b);

ml_hashmap hm_new(size_t key_sz, size_t val_sz, hm_hash_fn hash, hm_eq_fn eq);
void       hm_free(ml_hashmap m);

int    hm_put(ml_hashmap m, const void *key, const void *v);
void  *hm_get(const ml_hashmap m, const void *key);
int    hm_rm(ml_hashmap m, const void *key);
size_t hm_count(const ml_hashmap m);

typedef struct hm_iter {
    const struct ml_hashmap *m;
    size_t                   bucket;
    struct list_head        *next;
} hm_iter;

hm_iter hm_iterate(const ml_hashmap m);
int     hm_next(hm_iter *it, void **key, void **val);

#define FNV64_OFFSET 0xcbf29ce484222325ULL
#define FNV64_PRIME  0x100000001b3ULL

static inline uint64_t fnv1a_hash(const void *data, size_t len) {
    const unsigned char *p = data;
    uint64_t             h = FNV64_OFFSET;

    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= FNV64_PRIME;
    }

    return h;
}

#endif
