#ifndef NODEL_REHASHTABLE_H
#define NODEL_REHASHTABLE_H

#include <stdint.h>

#include "hashtable.h"

/* Malloc based resizable open addressing hashtable.
 * Automatically grows or shrinks the hashtable to meet capacity.
 * Amortized O(1) get, put, del, and more. Possible latency issues
 * when growing the hashtable.
 * (Consider iterative dual table migration? Meh.)
 * Automatically grows and shrinks to larger or smaller hashtables
 * based on usage.
 *
 * Keys must be %sizeof(int32_t).
 */

/* Stores the minimum hashtable size, pointer to current hashtable.
 *
 * Currently doubles capacity when (load > 3/4), halves capacity
 * when (load <= 3/16).
 */
typedef struct ndl_rhashtable_s {

    uint64_t min_size;
    ndl_hashtable *table;

} ndl_rhashtable;


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
ndl_rhashtable *ndl_rhashtable_init(uint64_t key_size, uint64_t val_size, uint64_t min_size);
void            ndl_rhashtable_kill(ndl_rhashtable *table);

ndl_rhashtable *ndl_rhashtable_minit(void *region, uint64_t key_size, uint64_t val_size, uint64_t min_size);
void            ndl_rhashtable_mkill(ndl_rhashtable *table);
uint64_t        ndl_rhashtable_msize(uint64_t key_size, uint64_t val_size, uint64_t min_size);

/* Get, put, delete items in the rhashtable.
 * When these return a pointer, the pointer is considered __INVALID__ on the
 * next table modifying operation. Use keys to index, not pointers.
 *
 * get() takes a key and returns a pointer to the associated value, or NULL.
 * put() takes a key/value pair and inserts or overrides the key with the given value.
 *     This returns a pointer to the value, or NULL if malloc() issues.
 *     If value is NULL, does not initialize value.
 *     If value is NULL, and overwriting, does not memset().
 * del() deletes a key from the hashtable.
 *    Returns 0 for success, and nonzero if the key is not present.
 */
void *ndl_rhashtable_get(ndl_rhashtable *table, void *key);
void *ndl_rhashtable_put(ndl_rhashtable *table, void *key, void *value);
int   ndl_rhashtable_del(ndl_rhashtable *table, void *key);

/* Iterate over elements of an rhashtable.
 * These pointers are considered __INVALID__ on the next table modifying operation.
 *
 * keyhead() gets the first key in the rhashtable.
 *     Returns NULL at end of list.
 * keynext() gets the next key in the rhashtable.
 *     Returns NULL at end of list. You can assume value = (((uint8_t *) key) + sizeof(key)).
 *
 * valhead() gets the first value in the rhashtable.
 *     Returns NULL at end of list.
 * valnext() gets the next value in the rhashtable.
 *     Returns NULL at end of list.
 */
void *ndl_rhashtable_keyhead(ndl_rhashtable *table);
void *ndl_rhashtable_keynext(ndl_rhashtable *table, void *last);

void *ndl_rhashtable_valhead(ndl_rhashtable *table);
void *ndl_rhashtable_valnext(ndl_rhashtable *table, void *last);

/* RHashtable metadata.
 *
 * min() gets the minimum number of buckets for the rhashtable.
 *
 * size() gets the number of used pairs in the hashtable.
 * cap() gets the capacity (in pairs) of the *current underlying hashtable*.
 *     This is not a meaningful number for most purposes.
 *
 * key_size() gets the sizeof(key) for the table.
 * val_size() gets the sizeof(value) for the table.
 */
uint64_t ndl_rhashtable_min (ndl_rhashtable *table);

uint64_t ndl_rhashtable_cap (ndl_rhashtable *table);
uint64_t ndl_rhashtable_size(ndl_rhashtable *table);

uint64_t ndl_rhashtable_key_size(ndl_rhashtable *table);
uint64_t ndl_rhashtable_val_size(ndl_rhashtable *table);

/* Print entirety of the current rhashtable. */
void ndl_rhashtable_print(ndl_rhashtable *table);

#endif /* NODEL_REHASHTABLE_H */
