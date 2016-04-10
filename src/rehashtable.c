#include "rehashtable.h"

#include <stdlib.h>
#include <stdio.h>

#include "hashtable.h"

struct ndl_rhashtable_s {

    uint64_t min_size;
    ndl_hashtable *table;
};


/* Create and destroy rhashtables.
 * Rhashtables must be killed before freed.
 *
 * init() allocates and initializes an rhashtable.
 *     if min_size is 0, picks sane default.
 * kill() cleans up and frees an rhashtable.
 *
 * minit() initializes an rhashtable from the given memory region.
 *     Memory region must be at least msize() in bytes.
 * mkill() cleans up an rhashtable, without free()ing the memory.
 * msize() gets the size of an rhashtable with the given parameters.
 */

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
    if (table != NULL)
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

/* If ((size/cap) > 3/4), realloc.
 * Computed as as (size*4 > cap*3).
 */
static inline void ndl_rhashtable_grow(ndl_rhashtable *table) {

    uint64_t size = ndl_hashtable_size(table->table);
    uint64_t cap = ndl_hashtable_cap(table->table);

    if ((size * 4) <= (cap * 3))
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

void *ndl_rhashtable_keyhead(ndl_rhashtable *table) {

    return ndl_hashtable_keyhead(table->table);
}

void *ndl_rhashtable_keynext(ndl_rhashtable *table, void *last) {

    return ndl_hashtable_keynext(table->table, last);
}

void *ndl_rhashtable_valhead(ndl_rhashtable *table) {

    return ndl_hashtable_valhead(table->table);

}

void *ndl_rhashtable_valnext(ndl_rhashtable *table, void *last) {

    return ndl_hashtable_valnext(table->table, last);
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
