#ifndef NODEL_NODEPOOL_H
#define NODEL_NODEPOOL_H

#include "node.h"

/* Temporary horrific implementation of a nodepool.
 * Global limit to nodecount, unlimited slots/KV pairs.
 * Either O(1) or O(slots) for most operations.
 */

/* TODO: Replace with old slab allocator for node IDs,
 * growable open-addressing hashtables for kv pairs.
 */

typedef struct ndl_kv_node_s {

    struct ndl_kv_node_s *next;

    ndl_sym key;
    ndl_value val;

} ndl_kv_node;

ndl_kv_node *ndl_kv_node_push(ndl_kv_node *node, ndl_sym key, ndl_value val); // returns node on error.
void ndl_kv_node_free(ndl_kv_node *node);

int ndl_kv_node_set(ndl_kv_node *node, ndl_sym key, ndl_value val);
ndl_value ndl_kv_node_get(ndl_kv_node *node, ndl_sym key);
ndl_kv_node *ndl_kv_node_remove(ndl_kv_node *node, ndl_sym key);

int ndl_kv_node_depth(ndl_kv_node *node);
ndl_sym ndl_kv_node_index(ndl_kv_node *node, int index);

#define NDL_MAX_NODES 4096
typedef struct ndl_node_pool_s {
    ndl_kv_node *slots[NDL_MAX_NODES];
} ndl_node_pool;

ndl_node_pool *ndl_node_pool_init(void);
void           ndl_node_pool_kill(ndl_node_pool *pool);

ndl_ref ndl_node_pool_alloc(ndl_node_pool *pool);
void    ndl_node_pool_free(ndl_node_pool *pool, ndl_ref node);

int       ndl_node_pool_set(ndl_node_pool *pool, ndl_ref node, ndl_sym key, ndl_value val);
int       ndl_node_pool_del(ndl_node_pool *pool, ndl_ref node, ndl_sym key);
ndl_value ndl_node_pool_get(ndl_node_pool *pool, ndl_ref node, ndl_sym key);

int ndl_node_pool_get_size(ndl_node_pool *pool, ndl_ref node);
ndl_sym ndl_node_pool_get_key(ndl_node_pool *pool, ndl_ref node, int index);

#endif /* NODEL_NODEPOOL_H */

