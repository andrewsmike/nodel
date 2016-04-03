#include "nodepool.h"

#include <stdlib.h>

nodel_kv_node *nodel_kv_node_push(nodel_kv_node *node, nodel_sym key, nodel_value val) {

    nodel_kv_node *head = (nodel_kv_node *) malloc(sizeof(nodel_kv_node));

    if (head == NULL)
        return node;

    head->next = node;
    head->key = key;
    head->val = val;

    return head;
}

void nodel_kv_node_free(nodel_kv_node *node) {

    while (node != NULL) {
        nodel_kv_node *next = node->next;
        free(node);
        node = next;
    }
}

int nodel_kv_node_set(nodel_kv_node *node, nodel_sym key, nodel_value val) {

    while (node != NULL && node->key != key)
        node = node->next;

    if (node == NULL)
        return -1;

    node->val = val;

    return 0;
}

nodel_value nodel_kv_node_get(nodel_kv_node *node, nodel_sym key) {

    while (node != NULL && node->key != key)
        node = node->next;

    if (node == NULL)
        return (nodel_value) {.type = ENONE, .ref = NODEL_NULL};

    return node->val;
}

nodel_kv_node *nodel_kv_node_remove(nodel_kv_node *node, nodel_sym key) {

    if (node == NULL)
        return NULL;

    if (node->key == key) {
        nodel_kv_node *next = node->next;
        free(node);
        return next;
    }

    while (node->next != NULL && node->next->key != key)
        node = node->next;

    if (node->next == NULL)
        return node;

    nodel_kv_node *next = node->next;
    node->next = node->next->next;
    free(next);

    return node;
}

int nodel_kv_node_depth(nodel_kv_node *node) {

    int depth;
    for (depth = 0; node != NULL; depth++)
        node = node->next;

    return depth;
}
nodel_sym nodel_kv_node_index(nodel_kv_node *node, int index) {

    while (index-->0 && node != NULL)
        node = node->next;

    if (node != NULL)
        return node->key;

    return NODEL_NULL_SYM;
}

nodel_node_pool *nodel_node_pool_init(void) {

    nodel_node_pool *pool = (nodel_node_pool*) malloc(sizeof(nodel_node_pool));

    if (pool == NULL)
        return NULL;

    for (int i = 0; i < NODEL_MAX_NODES; i++)
        pool->slots[i] = NULL;

    return pool;
}

void nodel_node_pool_kill(nodel_node_pool *pool) {

    for (int i = 0; i < NODEL_MAX_NODES; i++)
        if (pool->slots[i] != NULL)
            nodel_kv_node_free(pool->slots[i]);

    free(pool);
}

nodel_ref nodel_node_pool_alloc(nodel_node_pool *pool) {

    for (int i = 0; i < NODEL_MAX_NODES; i++) {
        if (pool->slots[i] == NULL) {
            pool->slots[i] = nodel_kv_node_push(NULL, *((uint64_t*)"refcount"), (nodel_value){.type=EINT, .num=1});
            return (nodel_ref) i;
        }
    }

    return NODEL_NULL;
}

int nodel_node_pool_set(nodel_node_pool *pool, nodel_ref node, nodel_sym key, nodel_value val) {

    if (node == NODEL_NULL)
        return -1;

    nodel_kv_node *head = pool->slots[node];

    int ret = nodel_kv_node_set(head, key, val);
    if (ret == 0)
        return ret;

    nodel_kv_node *t = nodel_kv_node_push(head, key, val);

    if (t == head)
        return -1;

    pool->slots[node] = t;

    return 0;
}

nodel_value nodel_node_pool_get(nodel_node_pool *pool, nodel_ref node, nodel_sym key) {

    if (node == NODEL_NULL)
        return (nodel_value) {.type=ENONE, .ref=NODEL_NULL};

    return nodel_kv_node_get(pool->slots[node], key);
}

int nodel_node_pool_get_size(nodel_node_pool *pool, nodel_ref node) {

    if (node == NODEL_NULL)
        return -1;

    return nodel_kv_node_depth(pool->slots[node]);
}

nodel_sym nodel_node_pool_get_key(nodel_node_pool *pool, nodel_ref node, int index) {

    if (node == NODEL_NULL)
        return -1;

    return nodel_kv_node_index(pool->slots[node], index);
}
