#include <assert.h>
#include <ecode.h>
#include <hashmap.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKET_CNT 16
#define LOAD_FACTOR        0.75f
#define SHOULD_RESIZE(hm)  ((hm)->entry_cnt > (hm)->bucket_cnt * LOAD_FACTOR)

struct hm_ent {
    size_t           hash;
    struct list_head siblings;
    void            *key;
    void            *val;
};

struct hm_bucket {
    struct list_head entries;
};

struct ml_hashmap {
    struct hm_bucket *buckets;
    size_t            bucket_cnt;
    size_t            entry_cnt;
    size_t            key_sz;
    size_t            val_sz;
    hm_hash_fn        hash_fn;
    hm_eq_fn          eq_fn;
};

ml_hashmap hm_new(size_t key_sz, size_t val_sz, hm_hash_fn hash, hm_eq_fn eq) {
    ml_hashmap map;

    assert(key_sz);
    assert(val_sz);
    assert(hash);
    assert(eq);

    map = malloc(sizeof(struct ml_hashmap));
    if (!map) { return NULL; }

    map->key_sz     = key_sz;
    map->val_sz     = val_sz;
    map->hash_fn    = hash;
    map->eq_fn      = eq;
    map->bucket_cnt = INITIAL_BUCKET_CNT;
    map->entry_cnt  = 0;
    map->buckets    = malloc(sizeof(struct hm_bucket) * map->bucket_cnt);

    for (size_t i = 0; i < map->bucket_cnt; i++) { list_init(&map->buckets[i].entries); }

    return map;
}

#ifdef MLIB_DEBUG
static size_t hm_count_chains(const struct ml_hashmap *m) {
    size_t n = 0;
    for (size_t i = 0; i < m->bucket_cnt; i++) {
        struct list_head *p;
        for (p = m->buckets[i].entries.next; p != &m->buckets[i].entries; p = p->next) n++;
    }
    return n;
}
#endif

void hm_free(ml_hashmap m) {
    assert(m);

    size_t seen = 0;


    for (size_t i = 0; i < m->bucket_cnt; i++) {
        struct hm_ent *e, *n;
        list_for_each_entry_safe(e, n, &m->buckets[i].entries, siblings) {
            seen++;
            free(e->key);
            free(e->val);
            free(e);
        }
    }

    assert(seen == m->entry_cnt);

    free(m->buckets);
    m->bucket_cnt = 0;
    free(m);
}

static struct hm_bucket *get_bucket(const ml_hashmap m, size_t hash) {
    return &m->buckets[hash & (m->bucket_cnt - 1)];
}

static struct hm_ent *get_ent(const ml_hashmap m, const size_t hash, const void *key) {
    struct hm_bucket *b;
    struct hm_ent    *e;

    b = get_bucket(m, hash);
    if (list_is_empty(&b->entries)) { return NULL; }

    list_for_each_entry(e, &b->entries, siblings) {
        if (e->hash == hash && m->eq_fn(e->key, key)) { return e; }
    }

    return NULL;
}

static int grow(ml_hashmap m) {
    struct hm_bucket *nb;
    size_t            new_cnt = m->bucket_cnt * 2;
    if (new_cnt < m->bucket_cnt || new_cnt > SIZE_MAX / sizeof(struct hm_bucket)) { return EC_RUNTIMEERR; }

    nb = malloc(new_cnt * sizeof(struct hm_bucket));
    if (!nb) { return EC_ALLOCERR; }

    for (size_t i = 0; i < new_cnt; i++) { list_init(&nb[i].entries); }

    for (size_t i = 0; i < m->bucket_cnt; i++) {
        struct hm_ent *e, *tmp;
        list_for_each_entry_safe(e, tmp, &m->buckets[i].entries, siblings) {
            struct hm_bucket *dst = &nb[e->hash & (new_cnt - 1)];
            list_move(&e->siblings, &dst->entries);
        }
    }

    free(m->buckets);
    m->buckets    = nb;
    m->bucket_cnt = new_cnt;
    return EC_NORMAL;
}

int hm_put(ml_hashmap m, const void *key, const void *v) {
    assert(m);
    assert(key);
    assert(v);

    size_t            hash = m->hash_fn(key);
    struct hm_bucket *b;
    struct hm_ent    *e = get_ent(m, hash, key);

    if (e) {
        memcpy(e->val, v, m->val_sz);
        return EC_NORMAL;
    }

    b = get_bucket(m, hash);
    e = malloc(sizeof(struct hm_ent));
    if (!e) { return EC_ALLOCERR; }

    list_init(&e->siblings);
    e->hash = hash;

    e->key = malloc(m->key_sz);
    if (!e->key) { goto fail0; }
    memcpy(e->key, key, m->key_sz);

    e->val = malloc(m->val_sz);
    if (!e->val) { goto fail1; }
    memcpy(e->val, v, m->val_sz);

    list_add_tail(&e->siblings, &b->entries);
    m->entry_cnt++;

    if (SHOULD_RESIZE(m)) { grow(m); }


#ifdef MLIB_DEBUG
    if (hm_count_chains(m) != m->entry_cnt) {
        fprintf(stderr,
                "lost entries on put #%zu (cnt=%zu buckets=%zu chains=%zu)\n",
                m->entry_cnt,
                m->entry_cnt,
                m->bucket_cnt,
                hm_count_chains(m));
        abort();
    }
#endif

    return EC_NORMAL;

fail1:
    free(e->key);
fail0:
    free(e);
    return EC_ALLOCERR;
}

void *hm_get(const ml_hashmap m, const void *key) {
    assert(m);
    assert(key);

    size_t         hash = m->hash_fn(key);
    struct hm_ent *e    = get_ent(m, hash, key);
    if (e) { return e->val; }
    return NULL;
}

int hm_rm(ml_hashmap m, const void *key) {
    assert(m);
    assert(key);

    size_t         hash = m->hash_fn(key);
    struct hm_ent *e    = get_ent(m, hash, key);

    if (!e) { return 0; }

    list_del(&e->siblings);
    free(e->key);
    free(e->val);
    free(e);
    m->entry_cnt--;
    return 1;
}

size_t hm_count(const ml_hashmap m) {
    assert(m);
    return m->entry_cnt;
}


static void hm_iter_seek(struct hm_iter *it, size_t b) {
    for (; b < it->m->bucket_cnt; b++) {
        if (!list_is_empty(&it->m->buckets[b].entries)) {
            it->bucket = b;
            it->next   = it->m->buckets[b].entries.next;
            return;
        }
    }
    it->next = NULL;
}

hm_iter hm_iterate(const ml_hashmap m) {
    assert(m);
    struct hm_iter it = {.m = m};
    hm_iter_seek(&it, 0);
    return it;
}

int hm_next(hm_iter *it, void **key, void **val) {
    assert(it);

    if (!it->next) { return 0; }

    struct hm_ent    *e    = container_of(it->next, struct hm_ent, siblings);
    struct list_head *head = &it->m->buckets[it->bucket].entries;
    if (e->siblings.next != head) {
        it->next = e->siblings.next;
    } else {
        hm_iter_seek(it, it->bucket + 1);
    }

    if (key) { *key = e->key; }
    if (val) { *val = e->val; }
    return 1;
}
