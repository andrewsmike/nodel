#include "rehashtable.h"

#include <stdlib.h>
#include <stdio.h>

#include "hashtable.h"

#define NDL_REHASHTABLE_MIN_DEFAULT 8

ndl_rhashtable *ndl_rhashtable_init(uint64_t key_size, uint64_t val_size, uint64_t min_size) {

    ndl_rhashtable *rtable = malloc(sizeof(ndl_rhashtable));
    if (rtable == NULL)
        return NULL;

    if (min_size == 0)
        min_size = NDL_REHASHTABLE_MIN_DEFAULT;

    ndl_hashtable *table = ndl_hashtable_init(key_size, val_size, min_size);
    if (table == NULL) {
        free(rtable);
        return NULL;
    }

    rtable->min_size = min_size;
    rtable->table = table;

    return rtable;
}

void ndl_rhashtable_kill(ndl_rhashtable *table) {

    ndl_rhashtable_mkill(table);
    free(table);

    return;
}

ndl_rhashtable *ndl_rhashtable_minit(void *region, uint64_t key_size, uint64_t val_size, uint64_t min_size) {

    ndl_rhashtable *rtable = (ndl_rhashtable *) region;
    if (rtable == NULL)
        return NULL;

    if (min_size == 0)
        min_size = NDL_REHASHTABLE_MIN_DEFAULT;

    ndl_hashtable *table = ndl_hashtable_init(key_size, val_size, min_size);
    if (table == NULL) {
        free(rtable);
        return NULL;
    }

    rtable->min_size = min_size;
    rtable->table = table;

    return rtable;
}

void ndl_rhashtable_mkill(ndl_rhashtable *table) {

    if (table == NULL)
        return;

    if (table->table != NULL)
        ndl_hashtable_kill(table->table);

    return;
}

uint64_t ndl_rhashtable_msize(uint64_t key_size, uint64_t val_size, uint64_t min_size) {

    return sizeof(ndl_rhashtable);
}

void *ndl_rhashtable_get(ndl_rhashtable *table, void *key) {

    return ndl_hashtable_get(table->table, key);
}

/* If ((size/cap) >= 3/4), realloc.
 * Computed as as (size*4 >= cap*3).
 */
static inline void ndl_rhashtable_grow(ndl_rhashtable *table) {

    uint64_t size = ndl_hashtable_size(table->table);
    uint64_t cap = ndl_hashtable_cap(table->table);

    if ((size * 4) < (cap * 3))
        return;

    uint64_t key_size = ndl_hashtable_key_size(table->table);
    uint64_t val_size = ndl_hashtable_val_size(table->table);

    ndl_hashtable *ntable = ndl_hashtable_init(key_size, val_size, cap * 2);
    if (ntable == NULL)
        return;

    int err = ndl_hashtable_copy(ntable, table->table);
    if (err != 0) {
        ndl_hashtable_kill(ntable);
        return;
    }

    ndl_hashtable_kill(table->table);
    table->table = ntable;

    return;
}

/* If ((size/cap) <= 3/16), realloc.
 * Computed as as (size*16 <= cap*3).
 */
static inline void ndl_rhashtable_shrink(ndl_rhashtable *table) {

    uint64_t size = ndl_hashtable_size(table->table);
    uint64_t cap = ndl_hashtable_cap(table->table);

    if (cap/2 < table->min_size)
        return;

    if ((size * 16) > (cap * 3))
        return;

    uint64_t key_size = ndl_hashtable_key_size(table->table);
    uint64_t val_size = ndl_hashtable_val_size(table->table);

    ndl_hashtable *ntable = ndl_hashtable_init(key_size, val_size, cap / 2);
    if (ntable == NULL)
        return;

    int err = ndl_hashtable_copy(ntable, table->table);
    if (err != 0) {
        ndl_hashtable_kill(ntable);
        return;
    }

    ndl_hashtable_kill(table->table);
    table->table = ntable;

    return;
}

void *ndl_rhashtable_put(ndl_rhashtable *table, void *key, void *value) {

    ndl_rhashtable_grow(table);

    return ndl_hashtable_put(table->table, key, value);
}

int ndl_rhashtable_del(ndl_rhashtable *table, void *key) {

    int ret = ndl_hashtable_del(table->table, key);

    if (ret == 0)
        ndl_rhashtable_shrink(table);

    return ret;
}

void *ndl_rhashtable_pairs_head(ndl_rhashtable *table) {

    return ndl_hashtable_pairs_head(table->table);
}

void *ndl_rhashtable_pairs_next(ndl_rhashtable *table, void *prev) {

    return ndl_hashtable_pairs_next(table->table, prev);
}


void *ndl_rhashtable_pairs_key(ndl_rhashtable *table, void *curr) {

    return ndl_hashtable_pairs_key(table->table, curr);
}

void *ndl_rhashtable_pairs_val(ndl_rhashtable *table, void *curr) {

    return ndl_hashtable_pairs_val(table->table, curr);
}

uint64_t ndl_rhashtable_min(ndl_rhashtable *table) {

    return table->min_size;
}

uint64_t ndl_rhashtable_cap(ndl_rhashtable *table) {

    return ndl_hashtable_cap(table->table);
}

uint64_t ndl_rhashtable_size(ndl_rhashtable *table) {

    return ndl_hashtable_size(table->table);
}

uint64_t ndl_rhashtable_key_size(ndl_rhashtable *table) {

    return ndl_hashtable_key_size(table->table);
}

uint64_t ndl_rhashtable_val_size(ndl_rhashtable *table) {

    return ndl_hashtable_val_size(table->table);
}

void ndl_rhashtable_print(ndl_rhashtable *table) {

    printf("Printing resizable hashtable:\n");
    printf("Minimum size: %ld. Current hashtable:\n", table->min_size);
    ndl_hashtable_print(table->table);
}
