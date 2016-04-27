#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

ndl_hashtable *ndl_hashtable_init(uint64_t key_size, uint64_t val_size, uint64_t capacity) {

    void *region = malloc(ndl_hashtable_msize(key_size, val_size, capacity));

    if (region == NULL)
        return NULL;

    ndl_hashtable *ret = ndl_hashtable_minit(region, key_size, val_size, capacity);
    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_hashtable_kill(ndl_hashtable *table) {

    free(table);
}

int ndl_hashtable_copy(ndl_hashtable *to, ndl_hashtable *from) {

    if ((to == NULL) || (from == NULL))
        return -1;

    void *val;
    void *next = ndl_hashtable_pairs_head(from);
    while (next != NULL) {

        void *key = ndl_hashtable_pairs_key(from, next);
        val = ndl_hashtable_put(to, key, ((uint8_t *) key) + from->key_size);
        if (val == NULL)
            return -1;

        next = ndl_hashtable_pairs_next(from, next);
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

    memset(table->data, 0, (uint64_t) bucketsize * capacity);

    return table;
}

uint64_t ndl_hashtable_msize(uint64_t key_size, uint64_t val_size, uint64_t capacity) {

    return sizeof(ndl_hashtable) + (sizeof(ndl_hashtable_bucket) + key_size + val_size) * capacity;
}

static inline uint64_t ndl_hashtable_hash(ndl_hashtable *table, void *key) {

    uint64_t size = table->key_size;

    uint64_t sum = 0;
    uint64_t size_64 = size >> 3;
    uint64_t size_32 = size >> 2;
    uint64_t extra_word = size & 0x04;

    uint64_t i;
    for (i = 0; i < size_64; i++) {
        sum += ((uint64_t *) key)[i];
    }

    if (extra_word)
        sum += ((uint32_t *) key)[size_32 - 1];

    return (sum + (sum << 1)) % table->capacity;
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

    if (table->capacity == table->size)
        return NULL;

    uint64_t hash = ndl_hashtable_hash(table, key);

    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size;

    uint8_t *base = table->data;
    ndl_hashtable_bucket *start = (ndl_hashtable_bucket *) (base + (bucketsize * hash));
    ndl_hashtable_bucket *curr = start;
    ndl_hashtable_bucket *end = (ndl_hashtable_bucket *) (base + (bucketsize * table->capacity));

    do {

        uint8_t *currkey = ((uint8_t *) curr) + sizeof(ndl_hashtable_bucket);
        uint8_t *currval = currkey + table->key_size;

        if (curr->marker == 0) {
            curr->marker = 1;
            table->size++;
            memcpy(currkey, key, table->key_size);
            if (value == NULL)
                return currval;
            else
                return memcpy(currval, value, table->val_size);
        }

        if (curr->marker == 1) {
            if (!ndl_hashtable_keycmp(table, curr, key)) {
                if (value == NULL)
                    return currval;
                else
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
    table->size--;

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

void *ndl_hashtable_pairs_head(ndl_hashtable *table) {

    ndl_hashtable_bucket *buckethead =
        ndl_hashtable_bucketscan(table, (ndl_hashtable_bucket *) table->data);

    if (buckethead == NULL)
        return NULL;

    return buckethead;
}

void *ndl_hashtable_pairs_next(ndl_hashtable *table, void *prev) {

    if (prev == NULL)
        return NULL;

    ndl_hashtable_bucket *nextbucket = (ndl_hashtable_bucket *)
        (((uint8_t *) prev) + sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size);

    nextbucket = ndl_hashtable_bucketscan(table, nextbucket);

    if (nextbucket == NULL)
        return NULL;

    return nextbucket;
}

void *ndl_hashtable_pairs_key(ndl_hashtable *table, void *curr) {

    return ((uint8_t *) curr) + sizeof(ndl_hashtable_bucket);
}

void *ndl_hashtable_pairs_val(ndl_hashtable *table, void *curr) {

    return ((uint8_t *) curr) + sizeof(ndl_hashtable_bucket) + table->key_size;
}

uint64_t ndl_hashtable_cap(ndl_hashtable *table) {

    return table->capacity;
}

uint64_t ndl_hashtable_size(ndl_hashtable *table) {

    return table->size;
}

uint64_t ndl_hashtable_key_size(ndl_hashtable *table) {

    return table->key_size;
}

uint64_t ndl_hashtable_val_size(ndl_hashtable *table) {

    return table->val_size;
}

void ndl_hashtable_print(ndl_hashtable *table) {

    printf("Printing hashtable.\n");
    printf("Key, val, cap: %ld:%ld x %ld.\n", table->key_size, table->val_size, table->capacity);
    printf("Usage: %ld / %ld.\n", table->size, table->capacity);

    uint8_t *base = (uint8_t *) table->data;
    uint64_t bucketsize = sizeof(ndl_hashtable_bucket) + table->key_size + table->val_size;

    uint64_t i;
    for (i = 0; i < table->capacity; i++) {

        ndl_hashtable_bucket *bucket = (ndl_hashtable_bucket *) (base + (bucketsize * i));
        printf("[%3d]\n", bucket->marker);
    }
}
