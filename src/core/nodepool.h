#ifndef NODEL_NODEPOOL_H
#define NODEL_NODEPOOL_H

#include "node.h"

/* Pool of nodes used in a graph.
 * Effectively abstracts over a number of rehashtables and
 * indexes them arbitrarily.
 * nodemap is an embedded rhashtable mapping from
 * ndl_ref -> ndl_rhashtable.
 * Operations amortized O(1).
 * May be replaced in the future, or rhashtable improved
 * to remove O(n) latency spikes on resize.
 */

typedef struct ndl_node_pool_s {

    ndl_ref last_id;
    uint8_t nodemap[];

} ndl_node_pool;

/* Create and destroy nodepools.
 *
 * init() allocates and initializes a nodepool.
 * kill() frees and deletes a nodepool.
 *
 * minit() initializes a nodepool in the given region of memory.
 * msize() gives the size needed to store a nodepool.
 * mkill() cleans a nodepool in the given memory region.
 */
ndl_node_pool *ndl_node_pool_init(void);
void           ndl_node_pool_kill(ndl_node_pool *pool);

ndl_node_pool *ndl_node_pool_minit(void *region);
void           ndl_node_pool_mkill(ndl_node_pool *pool);
uint64_t       ndl_node_pool_msize(void);

/* Allocate and free nodes from the pool.
 *
 * alloc() allocates a node from the pool.
 *     Returns NDL_NULL_REF on failure.
 * alloc_pref() allocates a node with a given ID.
 *     Returns NDL_NULL_REF on failure.
 * free() frees a node from the pool.
 *     Returns nonzero on failure.
 */
ndl_ref ndl_node_pool_alloc     (ndl_node_pool *pool);
ndl_ref ndl_node_pool_alloc_pref(ndl_node_pool *pool, ndl_ref pref);
int     ndl_node_pool_free      (ndl_node_pool *pool, ndl_ref node);

/* Manipulate node key/values.
 *
 * get() returns a given nodes' value at key.
 *     Returns val.type == EVAL_NONE on error.
 * put() sets the given node.key = value.
 *     Returns nonzero on error.
 * del() deletes the given node's value at key.
 *     Returns nonzero on error, missing node, missing key.
 */

ndl_value ndl_node_pool_get(ndl_node_pool *pool, ndl_ref node, ndl_sym key);
int       ndl_node_pool_put(ndl_node_pool *pool, ndl_ref node, ndl_sym key, ndl_value val);
int       ndl_node_pool_del(ndl_node_pool *pool, ndl_ref node, ndl_sym key);

/* Node iteration and node-related metadata.
 * Iterators __INVALIDATED__ after mutating operations.
 *
 * head() gets the iterator for first node in the node pool.
 *     Returns NULL on error or end of list.
 * next() gets the iterator for the next node in the node pool.
 *     Returns NULL on error or end of list.
 * node() gets the node at the given iterator.
 *     Returns NDL_NULL_REF on error.
 *
 * size() gets the number of nodes in a pool.
 *     Returns 0 on error.
 */
void   *ndl_node_pool_head(ndl_node_pool *pool);
void   *ndl_node_pool_next(ndl_node_pool *pool, void *prev);
ndl_ref ndl_node_pool_node(ndl_node_pool *pool, void *curr);

uint64_t ndl_node_pool_size(ndl_node_pool *pool);

/* Node key/value iteration and metadata.
 * Iterators __INVALIDATED__ by mutating operations.
 *
 * node_pairs_head() gets the first key/value pair iterator for the node.
 *     Returns NULL on error, end of list.
 * node_pairs_head() gets the next key/value pair iterator for the node.
 *     Returns NULL on error, end of list.
 *
 * node_pairs_key() gets the key of a node pair iterator.
 *     Returns NDL_NULL_SYM on error.
 * node_pairs_val() gets the value of a node pair iterator.
 *     Returns val.type = EVAL_NONE on error.
 *
 * node_size() returns the number of pairs for a given node.
 * Returns 0 on error.
 */
void *ndl_node_pool_node_pairs_head(ndl_node_pool *pool, ndl_ref node);
void *ndl_node_pool_node_pairs_next(ndl_node_pool *pool, ndl_ref node, void *prev);

ndl_sym   ndl_node_pool_node_pairs_key(ndl_node_pool *pool, ndl_ref node, void *curr);
ndl_value ndl_node_pool_node_pairs_val(ndl_node_pool *pool, ndl_ref node, void *curr);

uint64_t ndl_node_pool_node_size(ndl_node_pool *pool, ndl_ref node);

/* Nodepool metadata.
 *
 * get_counter() gets the next id to be assigned by the nodepool.
 * set_counter sets the next id to be assigned by the nodepool.
 */
ndl_ref ndl_node_pool_get_counter(ndl_node_pool *pool);
void    ndl_node_pool_set_counter(ndl_node_pool *pool, ndl_ref counter);

/* Print the entirety of the pool. */
void ndl_node_pool_print(ndl_node_pool *pool);

#endif /* NODEL_NODEPOOL_H */

