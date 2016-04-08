#include "nodepool.h"

#include <stdlib.h>
#include <stdio.h>

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

    ndl_kv_node *curr = node;

    while (curr->next != NULL && curr->next->key != key)
        curr = curr->next;

    if (curr->next == NULL)
        return node;

    ndl_kv_node *next = curr->next;
    curr->next = curr->next->next;
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

    while (index-- > 0 && node != NULL) {
        node = node->next;
    }

    if (node == NULL)
        return NDL_NULL_SYM;

    return node->key;
}

void *ndl_kv_node_head(ndl_kv_node *node) {

    return (void *) node;
}

void *ndl_kv_node_next(void *last) {

    if (last == NULL)
        return NULL;

    ndl_kv_node *lastkv = last;

    return lastkv->next;
}

ndl_sym ndl_kv_node_key(void *curr) {

    if (curr == NULL)
        return NDL_NULL_SYM;

    ndl_kv_node *currkv = curr;

    return currkv->key;
}

ndl_value ndl_kv_node_val(void *curr) {

    if (curr == NULL)
        return NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF);

    ndl_kv_node *currkv = curr;

    return currkv->val;
}

void nld_kv_node_print(ndl_kv_node *node) {

    char keybuff[16], valbuff[16];
    keybuff[15] = valbuff[15] = '\0';

    while (node != NULL) {
        ndl_value_to_string(NDL_VALUE(EVAL_SYM, sym=node->key), 15, keybuff);
        ndl_value_to_string(node->val, 15, valbuff);
        printf("(%s:%s)\n", keybuff, valbuff);
        node = node->next;
    }
}

ndl_node_pool *ndl_node_pool_init(void) {

    ndl_node_pool *pool = (ndl_node_pool*) malloc(sizeof(ndl_node_pool));

    if (pool == NULL)
        return NULL;

    for (int i = 0; i < NDL_MAX_NODES; i++)
        pool->slots[i] = NULL;

    pool->use = 0;

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
            pool->slots[i] = ndl_kv_node_push(NULL, NDL_SYM("self    "), NDL_VALUE(EVAL_REF, ref=i));
            pool->use++;
            return (ndl_ref) i;
        }
    }

    return NDL_NULL_REF;
}

ndl_ref ndl_node_pool_alloc_pref(ndl_node_pool *pool, ndl_ref pref) {

    if (pool->slots[(int) pref] == NULL) {
        pool->slots[(int) pref] = ndl_kv_node_push(NULL, NDL_SYM("self    "), NDL_VALUE(EVAL_REF, ref=pref));
        pool->use++;
        return pref;
    } else {
        return NDL_NULL_REF;
    }
}

void ndl_node_pool_free(ndl_node_pool *pool, ndl_ref node) {

    if (node == NDL_NULL_REF)
        return;

    ndl_kv_node_free(pool->slots[node]);

    pool->use--;
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

int ndl_node_pool_size(ndl_node_pool *pool) {

    return pool->use;
}

static inline ndl_ref ndl_node_pool_scan(ndl_node_pool *pool, ndl_ref start) {

    int i;
    for (i = (int) start; i < NDL_MAX_NODES; i++)
        if (pool->slots[i] != NULL)
            return (ndl_ref) i;

    return -1;
}

ndl_ref ndl_node_pool_head(ndl_node_pool *pool) {

    return ndl_node_pool_scan(pool, (ndl_ref) 0);
}

ndl_ref ndl_node_pool_next(ndl_node_pool *pool, ndl_ref last) {

    if (last == NDL_NULL_REF)
        return -1;

    if (last >= (NDL_MAX_NODES - 1))
        return -1;

    return ndl_node_pool_scan(pool, last + 1);
}

void *ndl_node_pool_kv_head(ndl_node_pool *pool, ndl_ref node) {

    if (node == NDL_NULL_REF)
        return NULL;

    if (node > NDL_MAX_NODES)
        return NULL;

    return ndl_kv_node_head(pool->slots[(int) node]);
}

void *ndl_node_pool_kv_next(ndl_node_pool *pool, ndl_ref node, void *last) {

    return ndl_kv_node_next(last);
}

ndl_sym ndl_node_pool_kv_key(ndl_node_pool *pool, ndl_ref node, void *curr) {

    return ndl_kv_node_key(curr);
}

ndl_value ndl_node_pool_kv_val(ndl_node_pool *pool, ndl_ref node, void *curr) {

    return ndl_kv_node_val(curr);
}

void ndl_node_pool_print(ndl_node_pool *pool) {

    printf("Printing nodepool.\n");

    for (int i = 0; i < NDL_MAX_NODES; i++) {

        if (pool->slots[i] != NULL) {

            printf("Printing node %d.\n", i);
            nld_kv_node_print(pool->slots[i]);
        }
    }
}
