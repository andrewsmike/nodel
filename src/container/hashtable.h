#ifndef NODEL_HASHTABLE_H
#define NODEL_HASHTABLE_H

#include <stdint.h>

/* Statically sized open addressing hashtable.
 * Used in construction of fancier things (resizable hashtable,
 * hashtable accelerated search tree, etc.)
 */

/* Simple hashtable datastructure.
 * Entire hashtable allocated as one block.
 * Tracks key and value size, bucket count and usage.
 * data is an array of buckets. Each bucket stores
 * a marker, a key, and a value. A marker is 1 for used,
 * 0 for unused, and -1 for deleted. When searching for
 * a key, you must search until either key is found, a
 * bucket with marker=0 is found, or you loop the entire
 * list.
 * Keys must be %sizeof(int32_t).
 */
typedef struct ndl_hashtable_bucket_s {

    int32_t marker;
    uint8_t data[];

} ndl_hashtable_bucket;

typedef struct ndl_hashtable_s {

    uint64_t key_size, val_size;
    uint64_t size, capacity;

    uint8_t data[];

} ndl_hashtable;


/* Create and destroy hashtables.
 * Hashtables can be freed directly, without any cleanup.
 *
 * init() allocates and initializes a hashtable.
 * kill() deletes a hashtable.
 * copy() copies the pairs of a hashtable into another of a different size.
 *     Used for growing or shrinking hashtables.
 *     Returns 0 on success, -1 on failure. Overrides existing keys.
 *
 * minit() initializes a hashtable from given memory region.
 *     Memory region must be at least msize() in bytes.
 *     No cleanup is required on deletion. (Except as needed by the element data.)
 * msize() gets the size needed for a hashtable with the given parameters.
 */
ndl_hashtable *ndl_hashtable_init(uint64_t key_size, uint64_t val_size, uint64_t capacity);
void           ndl_hashtable_kill(ndl_hashtable *table);
int            ndl_hashtable_copy(ndl_hashtable *to, ndl_hashtable *from);

ndl_hashtable *ndl_hashtable_minit(void *region, uint64_t key_size, uint64_t val_size, uint64_t capacity);
uint64_t       ndl_hashtable_msize(uint64_t key_size, uint64_t val_size, uint64_t capacity);

/* Get, put, delete items in the hashtable.
 *
 * get() takes a key and returns a pointer to the associated value, or NULL.
 * put() takes a key/value pair and inserts or overrides the key with the given value.
 *     This returns a pointer to the value, or NULL if capacity issues.
 *     If value is NULL, does not initialize value.
 *     If value is NULL, and replacing, does not memset.
 * del() deletes a key from the hashtable.
 *     Returns 0 for success, and nonzero if the key is not present.
 */
void *ndl_hashtable_get(ndl_hashtable *table, void *key);
void *ndl_hashtable_put(ndl_hashtable *table, void *key, void *value);
int   ndl_hashtable_del(ndl_hashtable *table, void *key);

/* Iterate over elements of an rhashtable.
 * These iterators are considered __INVALID__ after the next table modifying operation.
 *
 * pairs_head() gets the first key/value pair in the rhashtable.
 *     Returns NULL on error, end of list.
 * pairs_next() gets the next key/value pair in the hashtable.
 *     Returns NULL on error, end of list.
 *
 * pairs_key() gets the key for the pair iterator.
 *     Returns NULL on error.
 * pairs_val() gets the value for the pair iterator.
 *     Returns NULL on error.
 */
void *ndl_hashtable_pairs_head(ndl_hashtable *table);
void *ndl_hashtable_pairs_next(ndl_hashtable *table, void *prev);

void *ndl_hashtable_pairs_key(ndl_hashtable *table, void *curr);
void *ndl_hashtable_pairs_val(ndl_hashtable *table, void *curr);

/* Hashtable metadata.
 *
 * size() gets the number of used pairs in the hashtable.
 * cap()  gets the capacity (in pairs) of a hashtable.
 *
 * key_size() gets the key_size for the hashtable.
 * val_size() gets the val_size for the hashtable.
 */
uint64_t ndl_hashtable_size(ndl_hashtable *table);
uint64_t ndl_hashtable_cap (ndl_hashtable *table);

uint64_t ndl_hashtable_key_size(ndl_hashtable *table);
uint64_t ndl_hashtable_val_size(ndl_hashtable *table);


/* Print entirety of a hashtable. */
void ndl_hashtable_print(ndl_hashtable *table);

#endif /* NODEL_HASHTABLE_H */
