#ifndef NODEL_NODEPOOL_H
#define NODEL_NODEPOOL_H

#include "node.h"

/* Temporary horrific implementation of a nodepool.
 * Global limit to nodecount, unlimited slots/KV pairs.
 * Either O(1) or O(slots) for most operations.
 */

typedef struct nodel_kv_node_s {

    struct nodel_kv_node_s *next;

    nodel_sym key;
    nodel_value val;

} nodel_kv_node;

nodel_kv_node *nodel_kv_node_push(nodel_kv_node *node, nodel_sym key, nodel_value val); // returns node on error.
void nodel_kv_node_free(nodel_kv_node *node);

int nodel_kv_node_set(nodel_kv_node *node, nodel_sym key, nodel_value val);
nodel_value nodel_kv_node_get(nodel_kv_node *node, nodel_sym key);
nodel_kv_node *nodel_kv_node_remove(nodel_kv_node *node, nodel_sym key);

int nodel_kv_node_depth(nodel_kv_node *node);
nodel_sym nodel_kv_node_index(nodel_kv_node *node, int index);



#define NODEL_MAX_NODES 4096
struct nodel_node_pool_s {
    nodel_kv_node *slots[NODEL_MAX_NODES];
};

#endif /* NODEL_NODEPOOL_H */

