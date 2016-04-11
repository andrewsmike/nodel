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

static uint8_t ndl_hashtable_lookup_table[256] = {
    0x50, 0xa2, 0x96, 0xb6, 0x8f, 0x34, 0x24, 0x1f, 0x0e, 0x4e, 0x07, 0x01, 0xa6, 0xf6, 0xb8, 0xe1,
    0x3c, 0x60, 0xc2, 0xf1, 0x46, 0x78, 0x0b, 0xbd, 0xba, 0xd2, 0x31, 0x58, 0xe7, 0x9c, 0x12, 0x39,
    0x7a, 0xec, 0xe6, 0x80, 0xa9, 0x2c, 0x8a, 0xe5, 0x85, 0xbe, 0xb9, 0x5d, 0x37, 0x00, 0x74, 0x17,
    0xf3, 0x73, 0x93, 0xc3, 0x2b, 0x35, 0xc7, 0xbf, 0x4c, 0x0f, 0xd5, 0x2e, 0x6e, 0x1b, 0xc4, 0xf8,
    0x4a, 0xd8, 0x87, 0x4b, 0xc0, 0x7d, 0xfa, 0x94, 0xb3, 0x7c, 0x40, 0xa1, 0x7b, 0x99, 0x43, 0x51,
    0x9b, 0xb4, 0xfb, 0xfe, 0xfd, 0xe3, 0x54, 0x56, 0x11, 0x1e, 0x72, 0xdc, 0x0c, 0xcd, 0x6f, 0x69,
    0xa5, 0x04, 0x02, 0x22, 0x3d, 0x88, 0x57, 0x66, 0x86, 0x9e, 0x48, 0xca, 0xcc, 0x30, 0x59, 0x15,
    0x64, 0xde, 0xe4, 0x0d, 0xb0, 0x63, 0xd0, 0xbb, 0x26, 0x8d, 0x65, 0x36, 0xd7, 0x91, 0x16, 0x9d,
    0x09, 0xcb, 0x05, 0x55, 0x0a, 0x28, 0xdb, 0x77, 0xf7, 0x3e, 0x6d, 0x2a, 0x5f, 0x98, 0xcf, 0xc8,
    0xa8, 0xaa, 0x29, 0xab, 0x9a, 0x2f, 0xc5, 0xfc, 0xb5, 0x18, 0x52, 0x6a, 0x79, 0xe8, 0xa7, 0x62,
    0x06, 0xf5, 0x76, 0x45, 0x9f, 0x83, 0xad, 0x3f, 0x84, 0x95, 0x13, 0x68, 0x27, 0x03, 0x4d, 0xf0,
    0xb2, 0xd4, 0xea, 0x14, 0x38, 0x92, 0xef, 0xae, 0x1d, 0xd9, 0x1a, 0xd6, 0x49, 0x33, 0x67, 0xe0,
    0x1c, 0x7e, 0x47, 0x89, 0x70, 0xb1, 0xdd, 0x61, 0x75, 0xc9, 0x2d, 0xd1, 0x7f, 0xce, 0x08, 0xf2,
    0x4f, 0x5a, 0xbc, 0xac, 0xe9, 0xed, 0x5b, 0x32, 0x3b, 0xee, 0xa4, 0x21, 0x8c, 0xff, 0x10, 0x53,
    0xeb, 0xc6, 0xd3, 0xf9, 0xf4, 0x44, 0x19, 0x8e, 0xa0, 0x71, 0x23, 0x97, 0x6b, 0x6c, 0xda, 0x8b,
    0x20, 0x3a, 0x5e, 0x90, 0x82, 0x5c, 0xc1, 0xaf, 0x42, 0xdf, 0x41, 0x81, 0xe2, 0x25, 0xb7, 0xa3,
};

static inline uint64_t ndl_hashtable_hash(ndl_hashtable *table, void *key) {

    uint64_t size = table->key_size;

    uint64_t sum = 0;

    uint64_t i;
    for (i = 0; i < (size + 7)/8; i++) {
        uint64_t curr = ((uint64_t *) key)[i];

        uint64_t j;
        for (j = 0; j < 7; j++) {

            uint8_t *t = (uint8_t *) &curr;
            if (size < (i * 8 + j))
                t[j] = 0x00;

            t[j] = ndl_hashtable_lookup_table[t[j]];
        }

        sum += curr;
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
