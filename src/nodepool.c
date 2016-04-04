#include "nodepool.h"

#include <stdlib.h>

ndl_kv_node *ndl_kv_node_push(ndl_kv_node *node, ndl_sym key, ndl_value val) {

    ndl_kv_node *head = (ndl_kv_node *) malloc(sizeof(ndl_kv_node));

    if (head == NULL)
        return node;

    head->next = node;
    head->key = key;
    head->val = val;

    return head;
}

void ndl_kv_node_free(ndl_kv_node *node) {

    while (node != NULL) {
        ndl_kv_node *next = node->next;
        free(node);
        node = next;
    }
}

int ndl_kv_node_set(ndl_kv_node *node, ndl_sym key, ndl_value val) {

    while (node != NULL && node->key != key)
        node = node->next;

    if (node == NULL)
        return -1;

    node->val = val;

    return 0;
}

ndl_value ndl_kv_node_get(ndl_kv_node *node, ndl_sym key) {

    while (node != NULL && node->key != key)
        node = node->next;

    if (node == NULL)
        return (ndl_value) {.type = EVAL_NONE, .ref = NDL_NULL_REF};

    return node->val;
}

ndl_kv_node *ndl_kv_node_remove(ndl_kv_node *node, ndl_sym key) {

    if (node == NULL)
        return NULL;

    if (node->key == key) {
        ndl_kv_node *next = node->next;
        free(node);
        return next;
    }

    while (node->next != NULL && node->next->key != key)
        node = node->next;

    if (node->next == NULL)
        return node;

    ndl_kv_node *next = node->next;
    node->next = node->next->next;
    free(next);

    return node;
}

int ndl_kv_node_depth(ndl_kv_node *node) {

    int depth;
    for (depth = 0; node != NULL; depth++)
        node = node->next;

    return depth;
}
ndl_sym ndl_kv_node_index(ndl_kv_node *node, int index) {

    while (index-- > 0 && node != NULL)
        node = node->next;

    if (node != NULL)
        return node->key;

    return NDL_NULL_SYM;
}

ndl_node_pool *ndl_node_pool_init(void) {

    ndl_node_pool *pool = (ndl_node_pool*) malloc(sizeof(ndl_node_pool));

    if (pool == NULL)
        return NULL;

    for (int i = 0; i < NDL_MAX_NODES; i++)
        pool->slots[i] = NULL;

    return pool;
}

void ndl_node_pool_kill(ndl_node_pool *pool) {

    for (int i = 0; i < NDL_MAX_NODES; i++)
        if (pool->slots[i] != NULL)
            ndl_kv_node_free(pool->slots[i]);

    free(pool);
}

ndl_ref ndl_node_pool_alloc(ndl_node_pool *pool) {

    for (int i = 0; i < NDL_MAX_NODES; i++) {
        if (pool->slots[i] == NULL) {
            pool->slots[i] = ndl_kv_node_push(NULL, *((uint64_t*)"self    "), (ndl_value){.type=EVAL_REF, .ref=i});
            return (ndl_ref) i;
        }
    }

    return NDL_NULL_REF;
}

void ndl_node_pool_free(ndl_node_pool *pool, ndl_ref node) {

    if (node == NDL_NULL_REF)
        return;

    ndl_kv_node_free(pool->slots[node]);

    pool->slots[node] = NULL;

    return;
}

int ndl_node_pool_set(ndl_node_pool *pool, ndl_ref node, ndl_sym key, ndl_value val) {

    if (node == NDL_NULL_REF)
        return -1;

    ndl_kv_node *head = pool->slots[node];

    int ret = ndl_kv_node_set(head, key, val);
    if (ret == 0)
        return ret;

    ndl_kv_node *t = ndl_kv_node_push(head, key, val);

    if (t == head)
        return -1;

    pool->slots[node] = t;

    return 0;
}

int ndl_node_pool_del(ndl_node_pool *pool, ndl_ref node, ndl_sym key) {

    if (node == NDL_NULL_REF)
        return -1;

    pool->slots[node] = ndl_kv_node_remove(pool->slots[node], key);

    return 0;
}

ndl_value ndl_node_pool_get(ndl_node_pool *pool, ndl_ref node, ndl_sym key) {

    if (node == NDL_NULL_REF)
        return (ndl_value) {.type=EVAL_NONE, .ref=NDL_NULL_REF};

    return ndl_kv_node_get(pool->slots[node], key);
}

int ndl_node_pool_get_size(ndl_node_pool *pool, ndl_ref node) {

    if (node == NDL_NULL_REF)
        return -1;

    return ndl_kv_node_depth(pool->slots[node]);
}

ndl_sym ndl_node_pool_get_key(ndl_node_pool *pool, ndl_ref node, int index) {

    if (node == NDL_NULL_REF)
        return -1;

    return ndl_kv_node_index(pool->slots[node], index);
}
