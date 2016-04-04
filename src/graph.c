#include "graph.h"
#include "nodepool.h"

#include <stdlib.h>

struct ndl_graph_s {

    int64_t sweep;
    ndl_node_pool *pool;
};

ndl_graph *ndl_graph_init(void) {

    ndl_graph *ret = malloc(sizeof(ndl_graph));

    if (ret == NULL)
        return NULL;

    ret->pool = ndl_node_pool_init();

    if (ret->pool == NULL) {
        free(ret);
        return NULL;
    }

    ret->sweep = 0;

    return ret;
}

void ndl_graph_kill(ndl_graph *graph) {

    if (graph->pool != NULL)
        ndl_node_pool_kill(graph->pool);

    free(graph);

    return;
}

#define NDL_BACKREF(ref) (*((uint64_t*) "\0b\0\0\0\0b\0") | (((uint64_t) ref) << 16))
#define NDL_DEBACKREF(ref) (int32_t) ((ref & 0x0000FFFFFFFF0000) >> 16)
#define NDL_ISBACKREF(ref) ((ref & 0xFFFF00000000FFFF) == *((uint64_t*) "\0b\0\0\0\0b\0"))

ndl_ref ndl_graph_alloc(ndl_graph *graph) {

    ndl_ref ret = ndl_node_pool_alloc(graph->pool);

    if (ret == NDL_NULL_REF)
        return ret;

    int err = ndl_node_pool_set(graph->pool, ret, NDL_SYM("\0gcsweep"),
                                NDL_VALUE(EVAL_INT, num=-1));

    err |= ndl_node_pool_set(graph->pool, ret, NDL_BACKREF(ret),
                             NDL_VALUE(EVAL_INT, num=1));

    if (err != 0) {
        ndl_node_pool_free(graph->pool, ret);
        return NDL_NULL_REF;
    }

    return ret;
}

int ndl_graph_unmark(ndl_graph *graph, ndl_ref node) {

    return ndl_node_pool_set(graph->pool, node, NDL_SYM("\0gcsweep"),
                             NDL_VALUE(EVAL_INT, num=0));
}

int ndl_graph_mark(ndl_graph *graph, ndl_ref node) {

    return ndl_node_pool_set(graph->pool, node, NDL_SYM("\0gcsweep"),
                             NDL_VALUE(EVAL_INT, num=-1));
}

ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key) {

    ndl_ref ret = ndl_node_pool_alloc(graph->pool);

    if (ret == NDL_NULL_REF)
        return ret;

    int err = ndl_node_pool_set(graph->pool, ret, NDL_SYM("\0gcsweep"),
                                NDL_VALUE(EVAL_INT, num=0));

    err |= ndl_node_pool_set(graph->pool, ret, NDL_BACKREF(ret),
                             NDL_VALUE(EVAL_INT, ref=1));

    err |= ndl_node_pool_set(graph->pool, ret, NDL_BACKREF(base),
                             NDL_VALUE(EVAL_INT, ref=1));

    if (err == 0)
        err |= ndl_node_pool_set(graph->pool, base, key,
                                 NDL_VALUE(EVAL_REF, ref=ret));

    if (err != 0) {
        ndl_node_pool_free(graph->pool, ret);
        return NDL_NULL_REF;
    }

    return ret;
}

#define NDL_ISHIDDEN(sym) (((char*)&key)[0] == '\0')

static void ndl_graph_clean_mark(ndl_graph *graph, ndl_ref root, int sweep) {

    ndl_value gcsweep = ndl_node_pool_get(graph->pool, root, NDL_SYM("\0gcsweep"));
    if (gcsweep.type != EVAL_INT)
        return;

    if (gcsweep.num < 0 && (sweep >= 0))
        return;

    sweep = (sweep < 0)? -sweep : sweep; 

    if (gcsweep.num >= sweep)
        return;

    if (gcsweep.num >= 0) {

        gcsweep.num = sweep;

        ndl_node_pool_set(graph->pool, root, NDL_SYM("\0gcsweep"), gcsweep);
    }

    int count = ndl_node_pool_get_size(graph->pool, root);

    for (int i = 0; i < count; i++) {
        ndl_sym key = ndl_node_pool_get_key(graph->pool, root, i);
        if (!NDL_ISHIDDEN(key)) {

            ndl_value next = ndl_node_pool_get(graph->pool, root, key);
            if (next.type == EVAL_REF && next.ref != NDL_NULL_REF)
                ndl_graph_clean_mark(graph, next.ref, sweep);
        }
    }
}

static int ndl_graph_add_backref(ndl_node_pool *pool, ndl_ref from, ndl_ref to) {

    if (from == NDL_NULL_REF)
        return 0;

    ndl_value value = ndl_node_pool_get(pool, from, NDL_BACKREF(to));

    if (value.type == EVAL_NONE) {
        value.type = EVAL_INT;
        value.num = 1;
    } else if (value.type != EVAL_INT) {
        return -1;
    } else {
        value.num++;
    }

    return ndl_node_pool_set(pool, from, NDL_BACKREF(to), value);
}

static int ndl_graph_rm_backref(ndl_node_pool *pool, ndl_ref from, ndl_ref to) {

    if (from == NDL_NULL_REF)
        return 0;

    ndl_value value = ndl_node_pool_get(pool, from, NDL_BACKREF(to));

    if (value.type == EVAL_NONE) {
        return 0;
    }

    if (value.type != EVAL_INT)
        return -1;

    value.num--;

    if (value.num == 0) {
        return ndl_node_pool_del(pool, from, NDL_BACKREF(to));
    }

    return ndl_node_pool_set(pool, from, NDL_BACKREF(to), value);
}

static void ndl_graph_clean_remove(ndl_graph *graph, ndl_ref node) {

    int count = ndl_node_pool_get_size(graph->pool, node);

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key(graph->pool, node, i);

        if (!NDL_ISHIDDEN(key)) {
            ndl_value val = ndl_node_pool_get(graph->pool, node, key);
            if ((val.type == EVAL_REF) && (val.ref != NDL_NULL_REF))
                ndl_graph_rm_backref(graph->pool, val.ref, node);
        }
    }

    ndl_node_pool_free(graph->pool, node);
}

void ndl_graph_clean(ndl_graph *graph) {

    int sweep = ++graph->sweep;

    int i;
    for (i = 0; i < NDL_MAX_NODES; i++) {

        ndl_value gcsweep = ndl_node_pool_get(graph->pool, (ndl_ref) i, NDL_SYM("\0gcsweep"));

        if ((gcsweep.type == EVAL_INT) && (gcsweep.num == -1))
            ndl_graph_clean_mark(graph, (ndl_ref) i, -sweep);
    }

    for (i = 0; i < NDL_MAX_NODES; i++) {

        ndl_value gcsweep = ndl_node_pool_get(graph->pool, (ndl_ref) i, NDL_SYM("\0gcsweep"));

        if (gcsweep.type != EVAL_INT)
            continue;

        if ((gcsweep.num == -1) || (gcsweep.num >= sweep))
            continue;

        ndl_graph_clean_remove(graph, (ndl_ref) i);
    }
}

int ndl_graph_set(ndl_graph *graph, ndl_ref node, ndl_sym key, ndl_value value) {

    if (node == NDL_NULL_REF)
        return -1;

    ndl_value val = ndl_node_pool_get(graph->pool, node, key);

    int err = 0;
    if (value.type == EVAL_REF)
        err = ndl_graph_add_backref(graph->pool, value.ref, node);

    if (err != 0)
        return err;

    err = ndl_node_pool_set(graph->pool, node, key, value);

    if (err != 0) {
        if (value.type == EVAL_REF)
            ndl_graph_rm_backref(graph->pool, value.ref, node);
        return err;
    }

    if (val.type == EVAL_REF)
        ndl_graph_rm_backref(graph->pool, val.ref, node);

    return 0;
}

int ndl_graph_del(ndl_graph *graph, ndl_ref node, ndl_sym key) {

    if (node == NDL_NULL_REF)
        return -1;

    ndl_value val = ndl_node_pool_get(graph->pool, node, key);

    if (val.type == EVAL_REF)
        ndl_graph_rm_backref(graph->pool, val.ref, node);

    return ndl_node_pool_del(graph->pool, node, key);
}

int ndl_graph_size(ndl_graph *graph, ndl_ref node) {

    int count = ndl_node_pool_get_size(graph->pool, node);

    int sum = 0;

    for (int i = 0; i < count; i++) {
        ndl_sym key = ndl_node_pool_get_key(graph->pool, node, i);
        if (!NDL_ISHIDDEN(key))
            sum++;
    }

    return sum;
}

ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key) {

    return ndl_node_pool_get(graph->pool, node, key);
}

ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int index) {

    int count = ndl_node_pool_get_size(graph->pool, node);

    int sum = 0;

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key(graph->pool, node, i);

        if (!NDL_ISHIDDEN(key)) {
            if (sum == index)
                return key;
            sum++;
        }
    }

    return NDL_NULL_SYM;
}

int ndl_graph_backref_size(ndl_graph *graph, ndl_ref node) {

    int count = ndl_node_pool_get_size(graph->pool, node);

    int sum = 0;

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key(graph->pool, node, i);

        if (NDL_ISBACKREF(key))
            sum++;
    }

    return sum;
}

ndl_ref ndl_graph_backref_index(ndl_graph *graph, ndl_ref node, int index) {

    int count = ndl_node_pool_get_size(graph->pool, node);

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key(graph->pool, node, i);

        if (NDL_ISBACKREF(key)) {
            if (index == 0)
                return NDL_DEBACKREF(key);

            index--;
        }
    }

    return NDL_NULL_REF;
}

void ndl_graph_print(ndl_graph *graph) {
    ndl_node_pool_print(graph->pool);
}
