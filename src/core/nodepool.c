#include "nodepool.h"

#include "rehashtable.h"

#include <stdlib.h>
#include <stdio.h>

ndl_node_pool *ndl_node_pool_init(void) {

    void *region = malloc(ndl_node_pool_msize());
    if (region == NULL)
        return NULL;

    ndl_node_pool *res = ndl_node_pool_minit(region);
    if (res == NULL)
        free(region);

    return res;
}

void ndl_node_pool_kill(ndl_node_pool *pool) {

    ndl_node_pool_mkill(pool);
    free(pool);
}

ndl_node_pool *ndl_node_pool_minit(void *region) {

    ndl_node_pool *pool = (ndl_node_pool *) region;
    if (pool == NULL)
        return NULL;

    uint64_t node_size = ndl_rhashtable_msize(sizeof(ndl_sym), sizeof(ndl_value), 8);

    pool->last_id = 0;
    ndl_rhashtable *nodemap = ndl_rhashtable_minit((void *) pool->nodemap,
                                                   sizeof(ndl_ref), node_size, 128);
    if (nodemap == NULL)
        return NULL;

    return pool;
}

void ndl_node_pool_mkill(ndl_node_pool *pool) {

    void *curr = ndl_rhashtable_pairs_head((ndl_rhashtable *) pool->nodemap);
    while (curr != NULL) {

        void *node = ndl_rhashtable_pairs_val((ndl_rhashtable *) pool->nodemap, curr);

        ndl_rhashtable_mkill((ndl_rhashtable *) node);

        curr = ndl_rhashtable_pairs_next((ndl_rhashtable *) pool->nodemap, curr);
    }

    ndl_rhashtable_mkill((ndl_rhashtable *) pool->nodemap);
}

uint64_t ndl_node_pool_msize(void) {

    uint64_t node_size = ndl_rhashtable_msize(sizeof(ndl_sym), sizeof(ndl_value), 8);
    uint64_t nodemap_size = ndl_rhashtable_msize(sizeof(ndl_ref), node_size, 128);
    return sizeof(ndl_node_pool) + nodemap_size;
}

ndl_ref ndl_node_pool_alloc(ndl_node_pool *pool) {

    /* While last_id is taken, increment. */
    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    void *val = (void *) nodemap; /* Any non-NULL pointer. */
    while (val != NULL) {
        pool->last_id++;
        val = ndl_rhashtable_get(nodemap, &pool->last_id);
    }

    void *region = (ndl_rhashtable *) ndl_rhashtable_put(nodemap, &pool->last_id, NULL);
    if (region == NULL)
        return NDL_NULL_REF;

    ndl_rhashtable *new = ndl_rhashtable_minit(region, sizeof(ndl_sym), sizeof(ndl_value), 8);
    if (new == NULL) {
        ndl_rhashtable_del(nodemap, &pool->last_id);
        pool->last_id--;
        return NDL_NULL_REF;
    }

    return pool->last_id;
}

ndl_ref ndl_node_pool_alloc_pref(ndl_node_pool *pool, ndl_ref pref) {

    /* While last_id is taken, increment. */
    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    void *val = ndl_rhashtable_get(nodemap, &pref);
    if (val != NULL)
        return NDL_NULL_REF;

    void *region = (ndl_rhashtable *) ndl_rhashtable_put(nodemap, &pref, NULL);
    if (region == NULL)
        return NDL_NULL_REF;

    ndl_rhashtable *new = ndl_rhashtable_minit(region, sizeof(ndl_sym), sizeof(ndl_value), 8);
    if (new == NULL) {
        ndl_rhashtable_del(nodemap, &pref);
        return NDL_NULL_REF;
    }

    return pref;
}

int ndl_node_pool_free(ndl_node_pool *pool, ndl_ref node) {

    ndl_rhashtable *res = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (res != NULL)
        ndl_rhashtable_mkill(res);

    return ndl_rhashtable_del((ndl_rhashtable *) pool->nodemap, &node);
}

ndl_value ndl_node_pool_get(ndl_node_pool *pool, ndl_ref node, ndl_sym key) {

    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    ndl_rhashtable *res = (ndl_rhashtable *) ndl_rhashtable_get(nodemap, &node);
    if (res == NULL)
        return NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF);

    ndl_value *val = ndl_rhashtable_get(res, &key);
    if (val == NULL)
        return NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF);

    return *val;
}

int ndl_node_pool_put(ndl_node_pool *pool, ndl_ref node, ndl_sym key, ndl_value val) {

    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    ndl_rhashtable *res = (ndl_rhashtable *) ndl_rhashtable_get(nodemap, &node);
    if (res == NULL)
        return -1;

    void *slot = ndl_rhashtable_put(res, &key, &val);
    if (slot == NULL)
        return -1;

    return 0;
}

int ndl_node_pool_del(ndl_node_pool *pool, ndl_ref node, ndl_sym key) {

    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    ndl_rhashtable *res = (ndl_rhashtable *) ndl_rhashtable_get(nodemap, &node);
    if (res == NULL)
        return -1;

    return ndl_rhashtable_del(res, &key);
}

void *ndl_node_pool_head(ndl_node_pool *pool) {

    return ndl_rhashtable_pairs_head((ndl_rhashtable *) pool->nodemap);
}

void *ndl_node_pool_next(ndl_node_pool *pool, void *prev) {

    return ndl_rhashtable_pairs_next((ndl_rhashtable *) pool->nodemap, prev);
}

ndl_ref ndl_node_pool_node(ndl_node_pool *pool, void *curr) {

    ndl_ref *res = ndl_rhashtable_pairs_key((ndl_rhashtable *) pool->nodemap, curr);
    if (res == NULL)
        return NDL_NULL_REF;

    return *res;
}

uint64_t ndl_node_pool_size(ndl_node_pool *pool) {

    return ndl_rhashtable_size((ndl_rhashtable *) pool->nodemap);
}

void *ndl_node_pool_node_pairs_head(ndl_node_pool *pool, ndl_ref node) {

    void *res = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (res == NULL)
        return NULL;

    return ndl_rhashtable_pairs_head((ndl_rhashtable *) res);
}

void *ndl_node_pool_node_pairs_next(ndl_node_pool *pool, ndl_ref node, void *prev) {

    void *res = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (res == NULL)
        return NULL;

    return ndl_rhashtable_pairs_next((ndl_rhashtable *) res, prev);
}

ndl_sym ndl_node_pool_node_pairs_key(ndl_node_pool *pool, ndl_ref node, void *curr) {

    void *res = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (res == NULL)
        return NDL_NULL_SYM;

    ndl_sym *ret = ndl_rhashtable_pairs_key((ndl_rhashtable *) res, curr);
    if (ret == NULL)
        return NDL_NULL_SYM;
    else
        return *ret;
}

ndl_value ndl_node_pool_node_pairs_val(ndl_node_pool *pool, ndl_ref node, void *curr) {

    void *res = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (res == NULL)
        return NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF);

    ndl_value *ret = ndl_rhashtable_pairs_val((ndl_rhashtable *) res, curr);
    if (ret == NULL)
        return NDL_VALUE(EVAL_NONE, ref=NDL_NULL_SYM);
    else
        return *ret;
}

uint64_t ndl_node_pool_node_size(ndl_node_pool *pool, ndl_ref node) {

    ndl_rhashtable *nodeht = ndl_rhashtable_get((ndl_rhashtable *) pool->nodemap, &node);
    if (nodeht == NULL)
        return 0;

    return ndl_rhashtable_size(nodeht);
}

ndl_ref ndl_node_pool_get_counter(ndl_node_pool *pool) {

    return pool->last_id;
}

void ndl_node_pool_set_counter(ndl_node_pool *pool, ndl_ref counter) {

    pool->last_id = counter;
}

void ndl_node_pool_print(ndl_node_pool *pool) {

    printf("Printing pool.\n");
    printf("Last id: %ld.\n", pool->last_id);

    ndl_rhashtable *nodemap = (ndl_rhashtable *) pool->nodemap;

    void *curr = ndl_rhashtable_pairs_head(nodemap);
    while (curr != NULL) {

        ndl_ref *id = ndl_rhashtable_pairs_key(nodemap, curr);
        ndl_rhashtable *node = ndl_rhashtable_pairs_val(nodemap, curr);
        if ((id == NULL) || (node == NULL)) {
            fprintf(stderr, "Got invalid iterator. Failed to print nodepool.\n");
            return;
        }

        printf("Node %ld:\n", *id);
        void *pair = ndl_rhashtable_pairs_head(node);
        while (pair != NULL) {

            ndl_sym *key = ndl_rhashtable_pairs_key(node, pair);
            ndl_value *val = ndl_rhashtable_pairs_val(node, pair);

            char keybuff[16], valbuff[16];
            keybuff[15] = valbuff[15] = '\0';

            ndl_value_to_string(NDL_VALUE(EVAL_SYM, sym=*key), 15, keybuff);
            ndl_value_to_string(*val, 15, valbuff);

            printf("%s:%s\n", keybuff, valbuff);

            pair = ndl_rhashtable_pairs_next(node, pair);
        }

        curr = ndl_rhashtable_pairs_next(nodemap, curr);
    }
}
