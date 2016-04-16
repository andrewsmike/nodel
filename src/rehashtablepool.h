#ifndef NODEL_REHASHTABLEPOOL_H
#define NODEL_REHASHTABLEPOOL_H

#include <stdint.h>

#include "rehashtable.h"
#include "hashtable.h"
#include "slab.h"

/* Slab based pool of resizable hashtables.
 * Used to allocate resizable hashtables for
 * nodes. Allocation is amortized O(1),
 * put/get/del are amortized O(1).
 * When growing the hashtable, may have to perform O(n)
 * copy operation.
 *
 * Contains a list of slabs for allocating hashtables.
 * Each slab has twice the hashtable capacity of the last.
 * The last slab (total slabs == slab_count) holds pointers
 * to allocated hashtables, allowing the pool to handle
 * arbitrarily large hashtables.
 */
typedef struct ndl_rhashtable_pool_s {

    uint64_t key_size, val_size;
    uint64_t min_size;
    uint64_t slab_size, slab_count;

    uint8_t data[];

} ndl_rhashtable_pool;

/* Create and destroy rhashtable pools.
 * Rhashtable pools must be killed before freed.
 *
 * init() allocates the required memory and initializes the rhashtable_pool.
 *     if min_size and slabs are 0, picks sane defaults.
 * kill() cleans up and frees the rhashtable_pool.
 *
 * minit() initializes an rhashtable pool in the given memory region.
 *     Memory region must be at least msize() in bytes.
 * mkill() cleans up an rhashtable pool, without free()ing the memory.
 * msize() gets the size of an rhashtable pool with the given parameters.
 */
ndl_rhashtable_pool *ndl_rhashtable_pool_init(uint64_t key_size, uint64_t val_size,
                                              uint64_t min_size, uint64_t slabs);
void                 ndl_rhashtable_pool_kill(ndl_rhashtable_pool *pool);

ndl_rhashtable_pool *ndl_rhashtable_pool_minit(void *region, uint64_t key_size, uint64_t val_size,
                                               uint64_t min_size, uint64_t slabs);
void                 ndl_rhashtable_pool_mkill(ndl_rhashtable_pool *pool);
ndl_rhashtable_pool *ndl_rhashtable_pool_msize(uint64_t key_size, uint64_t val_size,
                                               uint64_t min_size, uint64_t slabs);

/* Get, put, and delete keys from an element in an rhashtable pool.
 * Returns a pointer to the new hashtable, if delete/put. If NULL, still using the
 * old hashtable.
 *
 * get() gets a pointer to the value with the given key from the given hashtable.
 *     Returns NULL on error, including key not found.
 * put() sets the value of the given key in the given hashtable.
 *     Returns NULL on error. Returns reference to new hashtable, in case it grew.
 * del() deletes the value of a given key in the given hashtable.
 *     Returns NULL on error. Returns reference to new hashtable, in case it shrunk.
 */
void          *ndl_rhashtable_pool_get(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key);
ndl_hashtable *ndl_rhashtable_pool_put(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key, void *value);
ndl_hashtable *ndl_rhashtable_pool_del(ndl_rhashtable_pool *pool, ndl_hashtable *elem, void *key);

/* Iterate over the hashtable slabs in an rhashtable pool.
 * The last slab stores a pointer to hashtables, rather than
 * hashtables itself. Check if ndl_slab_size() == sizeof(ndl_hashtable*).
 *
 * head() gets the first hashtable slab in the pool.
 *     Returns NULL at end of list.
 * next() gets the next hashtable slab in the pool.
 *     Returns NULL at end of list.
 */
ndl_slab *ndl_rhashtable_pool_head(ndl_rhashtable_pool *pool);
ndl_slab *ndl_rhashtable_pool_next(ndl_rhashtable_pool *pool, ndl_slab *prev);

/* RHashtable slab metadata.
 *
 * count() gets the number of dedicated slab sizes (before each table gets its own allocation.)
 *
 * min() gets the minimum hashtable size.
 * key_size() gets the sizeof(key) for each table.
 * val_size() gets the sizeof(value) for each table.
 */
uint64_t ndl_rhashtable_pool_slab_count(ndl_rhashtable_pool *pool);

uint64_t ndl_rhashtable_pool_min_size(ndl_rhashtable_pool *pool);
uint64_t ndl_rhashtable_pool_key_size(ndl_rhashtable_pool *pool);
uint64_t ndl_rhashtable_pool_val_size(ndl_rhashtable_pool *pool);

/* Print entirety of the rhashtable pool. */
void ndl_rhashtable_pool_print(ndl_rhashtable_pool *pool);

#endif /* NODEL_REHASHTABLEPOOL_H */
