#include "rehashtablepool.h"

#include <stdlib.h>
#include <stdio.h>

#include "hashtable.h"
#include "rehashtable.h"

#define NDL_REHASHTABLE_POOL_MIN_DEFAULT 8
#define NDL_REHASHTABLE_POOL_SLABS_DEFAULT 5

typedef struct ndl_rhashtable_pool_s {

    uint64_t key_size, val_size;
    uint64_t min_size;
    uint64_t slab_size, slab_count;

    uint8_t data[];

} ndl_rhashtable_pool;


static inline ndl_slab *ndl_rhashtable_pool_index_slab(ndl_rhashtable_pool *pool, int index) {

    return (ndl_slab *) (pool->data + (index * pool->htslab_size));
}

static inline int ndl_rhashtable_pool_slab_index(ndl_rhashtable_pool *pool, ndl_slab *slab) {

    return (int) ((((uint8_t *) slab) - pool->data) / pool->slab_size);
}

ndl_rhashtable_pool *ndl_rhashtable_pool_init(uint64_t key_size, uint64_t val_size,
                                              uint64_t min_size, uint64_t slabs) {

    void *region = malloc(ndl_rhashtable_pool_msize(key_size, val_size, min_size, slabs));
    if (region == NULL)
        return NULL;

    ndl_rhashtable_pool *ret = ndl_rhashtable_minit(region, key_size, val_size, min_size, slabs);

    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_rhashtable_pool_kill(ndl_rhashtable_pool *pool) {

    if (pool == NULL)
        return;

    ndl_rhashtable_pool_mkill(pool);

    return;
}

ndl_rhashtable_pool *ndl_rhashtable_pool_minit(void *region, uint64_t key_size, uint64_t val_size,
                                               uint64_t min_size, uint64_t slabs) {
    /* TODO: minit */
}

void ndl_rhashtable_pool_mkill(ndl_rhashtable_pool *pool) {

    uint64_t i;
    for (i = 0; i < (pool->slab_count - 1); i++) {

        ndl_slab_mkill(ndl_rhashtable_index_slab(pool, i));
    }

    ndl_slab *misc = ndl_rhashtable_index_slab(pool, pool->slab_count - 1);

    ndl_slab_index htindex = ndl_slab_head(misc);
    while (htindex != NDL_NULL_INDEX) {

        void *htp = ndl_slab_get(misc, htindex);
        if (htp != NULL)
            free(*htp);

        htindex = ndl_slab_next(misc, htindex);
    }

    ndl_slab_mkill(misc);
}

ndl_rhashtable_pool *ndl_rhashtable_pool_msize(uint64_t key_size, uint64_t val_size,
                                               uint64_t min_size, uint64_t slabs) {

    /* Slabs size is independent of the element size or block size.
     * There is a comment in slab.h linking to this line if that changes.
     */

    return sizeof(ndl_rhashtable_pool) + slabs * ndl_slab_msize(0, 0);
}

void *ndl_rhashtable_pool_get(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key) {

    return ndl_hashtable_get(elem, key);
}

static inline ndl_slab *ndl_rhashtable_pool_get_slab(ndl_rhashtable_pool *pool, uint64_t size) {

    uint64_t max = pool->slab_count * (pool->slab_size - 1);

    if (size >= max)
        return NULL;

    return ndl_rhashtable_pool_index_slab(pool, index);
}

static inline ndl_hashtable *ndl_rhashtable_pool_grow(ndl_rhashtable_pool *pool, ndl_hashtable *elem) {

    /* TODO: If uncomfortably large, alloc, copy, delete, return. */
    return elem;
}

static inline ndl_hashtable *ndl_rhashtable_pool_shrink(ndl_rhashtable_pool *pool, ndl_hashtable *elem) {

    /* TODO: If uncomfortably small, alloc, copy, delete, return. */
    return elem;
}

ndl_hashtable *ndl_rhashtable_pool_put(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key, void *value) {

    void *data = ndl_hashtable_put(elem, key, value);
    if (data == NULL)
        return NULL;

    elem = ndl_rhashtable_pool_grow(pool, elem);

    return elem;
}

ndl_hashtable *ndl_rhashtable_pool_del(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key) {

    int err = ndl_hashtable_del(elem, key, value);

    if (err == 0)
        return NULL;

    elem = ndl_rhashtable_pool_shrink(pool, elem);

    return elem;
}

ndl_slab *ndl_rhashtable_pool_head(ndl_rhashtable_pool *pool) {

    return (ndl_slab *) pool->data;
}

ndl_slab *ndl_rhashtable_pool_next(ndl_rhashtable_pool *pool, ndl_slab *prev) {

    uint8_t *next = ((uint8_t *) prev) + pool->slab_size;
    if (next >= (pool->data + (pool->slab_count * pool->slab_size)))
        return NULL;
    else
        return (ndl_slab *) next;
}

uint64_t ndl_rhashtable_pool_slab_count(ndl_rhashtable_pool *pool) {

    return pool->slab_count;
}

uint64_t ndl_rhashtable_pool_key_size(ndl_rhashtable_pool *pool) {

    return pool->key_size;
}

uint64_t ndl_rhashtable_pool_val_size(ndl_rhashtable_pool *pool) {

    return pool->val_size;
}

uint64_t ndl_rhashtable_pool_min_size(ndl_rhashtable_pool *pool) {

    return pool->min_size;
}

/* Print entirety of the rhashtable pool. */
void ndl_rhashtable_pool_print(ndl_rhashtable_pool *pool) {

    printf("Printing resizable hashtable pool.\n");
    printf("Key, value, slab size: %ld %ld %ld\n", pool->key_size, pool->val_size, pool->slab_size);
    printf("Minimum bucket count, number of slabs: %ld %ld.\n", pool->min_size, pool->slab_count);

    uint64_t i;
    for (i = 0; i < (pool->slab_count - 1); i++) {

        ndl_slab *slab = ndl_rhashtable_index_slab(pool, i);

        printf("Slab with bucket size %ld:\n" (pool->min_size << i));
        ndl_slab_print(slab);
        prinf("Each hashtable in this slab:\n");

        ndl_slab_index htindex = ndl_slab_head(slab);
        while (htindex != NDL_NULL_INDEX) {

            void *htp = ndl_slab_get(slab, htindex);
            if (htp != NULL)
                ndl_hashtable_print(*((ndl_hashtable **) htp));

            htindex = ndl_slab_next(slab, htindex);
        }
    }

    ndl_slab *misc = ndl_rhashtable_index_slab(pool, pool->slab_count - 1);

    printf("Slab with arbitrary bucket size:\n");

    ndl_slab_index htindex = ndl_slab_head(misc);
    while (htindex != NDL_NULL_INDEX) {

        void *htp = ndl_slab_get(misc, htindex);
        if (htp != NULL)
            ndl_hashtable_print(*((ndl_hashtable **) htp));

        htindex = ndl_slab_next(misc, htindex);
    }
}

