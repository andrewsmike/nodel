#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

ndl_hashtable *ndl_hashtable_init(uint64_t key_size, uint64_t val_size, uint64_t capacity) {

    ndl_hashtable *table = malloc(ndl_hashtable_msize(key_size, val_size, capacity));

    if (table == NULL)
        return NULL;

    return ndl_hashtable_minit(table, key_size, val_size, capacity);
}

void ndl_hashtable_kill(ndl_hashtable *table) {

    free(table);
}

int ndl_hashtable_copy(ndl_hashtable *to, ndl_hashtable *from) {

    if ((to == NULL) || (from == NULL))
        return -1;

    void *val;
    void *next = ndl_hashtable_keyhead(from);
    while (next != NULL) {

        val = ndl_hashtable_put(to, next, ((uint8_t *) next) + from->key_size);
        if (val == NULL)
            return -1;

        next = ndl_hashtable_keynext(from, next);
    }

    return 0;
}

ndl_hashtable *ndl_hashtable_minit(void *region, uint64_t key_size, uint64_t val_size, uint64_t capacity) {

    ndl_hashtable *table = region;

    table->key_size = key_size;
    table->val_size = val_size;

    table->size = 0;
    table->capacity = capacity;

    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + key_size + val_size;
    ndl_hashtable_bucket *bucket = (ndl_hashtable_bucket *) table->data;

    uint64_t i;
    for (i = 0; i < capacity; i++) {

        bucket->marker = 0;

        bucket = (ndl_hashtable_bucket *) (((uint8_t *) bucket) + bucketsize);
    }

    return table;
}

uint64_t ndl_hashtable_msize(uint64_t key_size, uint64_t val_size, uint64_t capacity) {

    return sizeof(ndl_hashtable) + (sizeof(ndl_hashtable_bucket) + key_size + val_size) * capacity;
}

static inline uint64_t ndl_hashtable_hash(ndl_hashtable *table, void *key) {

    uint64_t size = table->key_size;

    uint64_t sum = 0;

    uint64_t i;
    for (i = 0; i < size; i++) {

        sum += ((uint8_t *) key)[i];
    }

    return sum % table->capacity;
}

static inline int ndl_hashtable_keycmp(ndl_hashtable *table, ndl_hashtable_bucket *bucket, void *key) {

    void *bucketkey = ((uint8_t *) bucket) + sizeof(ndl_hashtable_bucket);

    return memcmp(bucketkey, key, table->key_size);
}

void *ndl_hashtable_get(ndl_hashtable *table, void *key) {

    uint64_t hash = ndl_hashtable_hash(table, key);

    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size;

    uint8_t *base = table->data;
    ndl_hashtable_bucket *start = (ndl_hashtable_bucket *) (base + (bucketsize * hash));
    ndl_hashtable_bucket *curr = start;
    ndl_hashtable_bucket *end = (ndl_hashtable_bucket *) (base + (bucketsize * table->capacity));

    do {

        if (curr->marker == 0)
            return NULL;

        if (curr->marker == 1)
            if (!ndl_hashtable_keycmp(table, curr, key))
                return ((uint8_t *) curr) + sizeof(ndl_hashtable_bucket) + table->key_size;

        curr = (ndl_hashtable_bucket *) (((uint8_t *) curr) + bucketsize);
        if (curr == end)
            curr = (ndl_hashtable_bucket *) base;

    } while (curr != start);

    return NULL;
}

void *ndl_hashtable_put(ndl_hashtable *table, void *key, void *value) {

    uint64_t hash = ndl_hashtable_hash(table, key);

    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size;

    uint8_t *base = table->data;
    ndl_hashtable_bucket *start = (ndl_hashtable_bucket *) (base + (bucketsize * hash));
    ndl_hashtable_bucket *curr = start;
    ndl_hashtable_bucket *end = (ndl_hashtable_bucket *) (base + (bucketsize * table->capacity));

    do {

        uint8_t *currkey = ((uint8_t *) curr) + sizeof(ndl_hashtable_bucket);
        uint8_t *currval = currval + table->key_size;

        if (curr->marker == 0) {
            curr->marker = 1;
            memcpy(currkey, key, table->key_size);
            return memcpy(currval, value, table->val_size);
        }

        if (curr->marker == 1) {
            if (!ndl_hashtable_keycmp(table, curr, key)) {
                return memcpy(currval, value, table->val_size);
            }
        }

        curr = (ndl_hashtable_bucket *) (((uint8_t *) curr) + bucketsize);
        if (curr == end)
            curr = (ndl_hashtable_bucket *) base;

    } while (curr != start);

    return NULL;
}

int ndl_hashtable_del(ndl_hashtable *table, void *key) {

    uint8_t *val = (uint8_t *) ndl_hashtable_get(table, key);

    if (val == NULL)
        return -1;

    ndl_hashtable_bucket *bucket = (ndl_hashtable_bucket *) (val - sizeof(ndl_hashtable_bucket) - table->key_size);

    bucket->marker = -1;

    return 0;
}

static inline ndl_hashtable_bucket *ndl_hashtable_bucketscan(ndl_hashtable *table, ndl_hashtable_bucket *bucket) {

    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size;
    ndl_hashtable_bucket *end = (ndl_hashtable_bucket *) (table->data + (bucketsize * table->capacity));

    while (bucket < end) {

        if (bucket->marker == 1)
            return bucket;

        bucket = (ndl_hashtable_bucket *) (((uint8_t *) bucket) + bucketsize);
    }

    return NULL;
}

void *ndl_hashtable_keyhead(ndl_hashtable *table) {

    ndl_hashtable_bucket *buckethead =
        ndl_hashtable_bucketscan(table, (ndl_hashtable_bucket *) table->data);

    if (buckethead == NULL)
        return NULL;

    return ((uint8_t *) buckethead) + sizeof(ndl_hashtable_bucket);
}

void *ndl_hashtable_keynext(ndl_hashtable *table, void *last) {

    if (last == NULL)
        return NULL;

    ndl_hashtable_bucket *nextbucket = (ndl_hashtable_bucket *)
        (((uint8_t *) last) + table->key_size + table->val_size);

    nextbucket = ndl_hashtable_bucketscan(table, nextbucket);

    if (nextbucket == NULL)
        return NULL;

    return ((uint8_t *) nextbucket) + sizeof(ndl_hashtable_bucket);
}

void *ndl_hashtable_valhead(ndl_hashtable *table) {

    ndl_hashtable_bucket *buckethead =
        ndl_hashtable_bucketscan(table, (ndl_hashtable_bucket *) table->data);

    if (buckethead == NULL)
        return NULL;

    return ((uint8_t *) buckethead) + sizeof(ndl_hashtable_bucket) + table->key_size;
}

void *ndl_hashtable_valnext(ndl_hashtable *table, void *last) {

    if (last == NULL)
        return NULL;

    ndl_hashtable_bucket *nextbucket = (ndl_hashtable_bucket *) (((uint8_t *) last) + table->val_size);

    nextbucket = ndl_hashtable_bucketscan(table, nextbucket);

    if (nextbucket == NULL)
        return NULL;

    return ((uint8_t *) nextbucket) + sizeof(ndl_hashtable_bucket) + table->key_size;
}

uint64_t ndl_hashtable_cap (ndl_hashtable *table) {

    return table->capacity;
}

uint64_t ndl_hashtable_size(ndl_hashtable *table) {

    return table->size;
}
