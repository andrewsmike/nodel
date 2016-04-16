#include "graph.h"
#include "nodepool.h"
#include "endian.h"

#include <stdlib.h>

typedef struct ndl_graph_s {

    int64_t sweep;
    uint8_t pool[];
} ndl_graph;

ndl_graph *ndl_graph_init(void) {

    void *region = malloc(ndl_graph_msize());
    if (region == NULL)
        return NULL;

    ndl_graph *ret = ndl_graph_minit(region);
    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_graph_kill(ndl_graph *graph) {

    ndl_graph_mkill(graph);

    free(graph);

    return;
}

ndl_graph *ndl_graph_minit(void *region) {

    ndl_graph *ret = (ndl_graph*) region;

    ndl_node_pool *pool = ndl_node_pool_minit((void *) ret->pool);

    if (pool == NULL)
        return NULL;

    ret->sweep = 0;

    return ret;
}

void ndl_graph_mkill(ndl_graph *graph) {

    ndl_node_pool_mkill((ndl_node_pool *) graph->pool);

    return;
}

uint64_t ndl_graph_msize(void) {

    return sizeof(ndl_graph) + ndl_node_pool_msize();
}

#define NDL_BACKREF(ref) (*((uint64_t*) "\0b\0\0\0\0b\0") | (((uint64_t) ref) << 16))
#define NDL_DEBACKREF(ref) (int32_t) ((ref & 0x0000FFFFFFFF0000) >> 16)
#define NDL_ISBACKREF(ref) ((ref & 0xFFFF00000000FFFF) == *((uint64_t*) "\0b\0\0\0\0b\0"))

ndl_ref ndl_graph_alloc(ndl_graph *graph) {

    ndl_ref ret = ndl_node_pool_alloc((ndl_node_pool *) graph->pool);

    if (ret == NDL_NULL_REF)
        return ret;

    int err = ndl_node_pool_set((ndl_node_pool *) graph->pool,
                                ret, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-1));

    err |= ndl_node_pool_set((ndl_node_pool *) graph->pool,
                             ret, NDL_BACKREF(ret), NDL_VALUE(EVAL_INT, num=1));

    if (err != 0) {
        ndl_node_pool_free((ndl_node_pool *) graph->pool, ret);
        return NDL_NULL_REF;
    }

    return ret;
}

int ndl_graph_stat(ndl_graph *graph, ndl_ref node) {

    ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                      node, NDL_SYM("\0gcsweep"));
    if (val.type != EVAL_INT)
        return -1;
    else
        return (val.num == -1)? 1 : 0;
}

int ndl_graph_unmark(ndl_graph *graph, ndl_ref node) {

    return ndl_node_pool_set((ndl_node_pool *) graph->pool,
                             node, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=0));
}

int ndl_graph_mark(ndl_graph *graph, ndl_ref node) {

    return ndl_node_pool_set((ndl_node_pool *) graph->pool,
                             node, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-1));
}

ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key) {

    ndl_ref ret = ndl_node_pool_alloc((ndl_node_pool *) graph->pool);

    if (ret == NDL_NULL_REF)
        return ret;

    int err = ndl_node_pool_set((ndl_node_pool *) graph->pool,
                                ret, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=0));

    err |= ndl_node_pool_set((ndl_node_pool *) graph->pool,
                             ret, NDL_BACKREF(ret), NDL_VALUE(EVAL_INT, ref=1));

    err |= ndl_node_pool_set((ndl_node_pool *) graph->pool,
                             ret, NDL_BACKREF(base), NDL_VALUE(EVAL_INT, ref=1));

    if (err == 0)
        err |= ndl_node_pool_set((ndl_node_pool *) graph->pool,
                                 base, key, NDL_VALUE(EVAL_REF, ref=ret));

    if (err != 0) {
        ndl_node_pool_free((ndl_node_pool *) graph->pool, ret);
        return NDL_NULL_REF;
    }

    return ret;
}

#define NDL_ISHIDDEN(sym) (((char*)&key)[0] == '\0')

static void ndl_graph_clean_mark(ndl_graph *graph, ndl_ref root, int64_t sweep) {

    ndl_value gcsweep = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                          root, NDL_SYM("\0gcsweep"));
    if (gcsweep.type != EVAL_INT)
        return;

    if (gcsweep.num < 0 && (sweep >= 0))
        return;

    sweep = (sweep < 0)? -sweep : sweep; 

    if (gcsweep.num >= sweep)
        return;

    if (gcsweep.num >= 0) {

        gcsweep.num = sweep;

        ndl_node_pool_set((ndl_node_pool *) graph->pool,
                          root, NDL_SYM("\0gcsweep"), gcsweep);
    }

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool, root);

    for (int i = 0; i < count; i++) {
        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool, root, i);
        if (!NDL_ISHIDDEN(key)) {

            ndl_value next = ndl_node_pool_get((ndl_node_pool *) graph->pool, root, key);
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

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool, node);

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool, node, i);

        if (!NDL_ISHIDDEN(key)) {
            ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool, node, key);
            if ((val.type == EVAL_REF) && (val.ref != NDL_NULL_REF))
                ndl_graph_rm_backref((ndl_node_pool *) graph->pool, val.ref, node);
        }
    }

    ndl_node_pool_free((ndl_node_pool *) graph->pool, node);
}

void ndl_graph_clean(ndl_graph *graph) {

    int64_t sweep = ++graph->sweep;

    int i;
    for (i = 0; i < NDL_MAX_NODES; i++) {

        ndl_value gcsweep = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                              (ndl_ref) i, NDL_SYM("\0gcsweep"));

        if ((gcsweep.type == EVAL_INT) && (gcsweep.num == -1))
            ndl_graph_clean_mark(graph, (ndl_ref) i, -sweep);
    }

    for (i = 0; i < NDL_MAX_NODES; i++) {

        ndl_value gcsweep = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                              (ndl_ref) i, NDL_SYM("\0gcsweep"));

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

    ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                      node, key);

    int err = 0;
    if (value.type == EVAL_REF)
        err = ndl_graph_add_backref((ndl_node_pool *) graph->pool,
                                    value.ref, node);

    if (err != 0)
        return err;

    err = ndl_node_pool_set((ndl_node_pool *) graph->pool,
                            node, key, value);

    if (err != 0) {
        if (value.type == EVAL_REF)
            ndl_graph_rm_backref((ndl_node_pool *) graph->pool,
                                 value.ref, node);
        return err;
    }

    if (val.type == EVAL_REF)
        ndl_graph_rm_backref((ndl_node_pool *) graph->pool,
                             val.ref, node);

    return 0;
}

int ndl_graph_del(ndl_graph *graph, ndl_ref node, ndl_sym key) {

    if (node == NDL_NULL_REF)
        return -1;

    ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                      node, key);

    if (val.type == EVAL_REF)
        ndl_graph_rm_backref((ndl_node_pool *) graph->pool,
                             val.ref, node);

    return ndl_node_pool_del((ndl_node_pool *) graph->pool,
                             node, key);
}

int64_t ndl_graph_size(ndl_graph *graph, ndl_ref node) {

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool,
                                       node);

    int sum = 0;

    for (int i = 0; i < count; i++) {
        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool,
                                            node, i);
        if (!NDL_ISHIDDEN(key))
            sum++;
    }

    return sum;
}

ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key) {

    return ndl_node_pool_get((ndl_node_pool *) graph->pool,
                             node, key);
}

ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int64_t index) {

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool,
                                       node);

    int sum = 0;

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool,
                                            node, i);

        if (!NDL_ISHIDDEN(key)) {
            if (sum == index)
                return key;
            sum++;
        }
    }

    return NDL_NULL_SYM;
}

int64_t ndl_graph_backref_size(ndl_graph *graph, ndl_ref node) {

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool,
                                       node);

    int sum = 0;

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool,
                                            node, i);

        if (NDL_ISBACKREF(key))
            sum++;
    }

    return sum;
}

ndl_ref ndl_graph_backref_index(ndl_graph *graph, ndl_ref node, int64_t index) {

    int count = ndl_node_pool_get_size((ndl_node_pool *) graph->pool,
                                       node);

    for (int i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool,
                                            node, i);

        if (NDL_ISBACKREF(key)) {
            if (index == 0)
                return NDL_DEBACKREF(key);

            index--;
        }
    }

    return NDL_NULL_REF;
}

/* Serialization format:
 * Entirely in big endian.
 * Nodes not ordered by ID.
 * KV pairs unordered.
 * Includes self pointer.
 *
 * uint32_t node_count
 * [
 *   uint32_t id
 *   uint16_t key_count
 *   [
 *     uint64_t key  # ((char*)&key)[0] = first letter
 *     uint8_t type
 *     uint64_t value # Same deal for symbols.
 *   ]
 * ]
 *
 *
 * Size:
 * sizeo(graphroot) = 4
 * sizeof(noderoot) = 6
 * sizeof(kvpair) = 17
 * avgsizeof(node) = sizeof(noderoot) + sizeof(kvpair) * avgkvpairs = 6 + 17*avgkvpairs
 * sizeof(graph) = sizeof(graphroot) + avgsizeof(node) * nodes = 4 + (6+17*avgkvpairs)*nodes
 *               = 4 + 6*nodes + 17*keys
 */

int64_t ndl_graph_mem_est(ndl_graph *graph) {

    int nodes = ndl_node_pool_size((ndl_node_pool *) graph->pool);

    /* Assume there are not more than 16 keys per node.
     * If there are, caller functions will just retry with more.
     * If there are not, it's not an unreasonable amount of wasted memory.
     */
    int estkvpairs = nodes * 16;

    return 4 + 6*nodes + 17*estkvpairs;
}

#define MEMPUSH(type, value)              \
    *((type *) &to[curr]) = (type) value; \
    curr += sizeof(type)

#define MEMPOP(type, var)          \
    var = *((type *) &from[curr]); \
    curr += sizeof(type)

static inline int64_t ndl_graph_to_mem_kvpair(ndl_graph *graph, ndl_sym key, ndl_value val, uint64_t maxlen, char *to) {

    uint64_t curr = 0;

    if ((maxlen - curr) < (sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint64_t)))
        return -1;

    MEMPUSH(uint64_t, ENDIAN_TO_BIG_64(key));
    MEMPUSH(uint8_t, ((uint8_t) val.type));
    MEMPUSH(int64_t, ENDIAN_TO_BIG_64(((uint64_t) val.num)));

    return (int64_t) curr;
}

static inline int64_t ndl_graph_to_mem_node(ndl_graph *graph, long int node, uint64_t maxlen, char *to) {

    uint64_t curr = 0;

    uint32_t id = (uint32_t) node;

    uint16_t count = (uint16_t) ndl_node_pool_get_size((ndl_node_pool *) graph->pool, (ndl_ref) node);

    if ((maxlen - curr) < (sizeof(id) + sizeof(count)))
        return -1;

    MEMPUSH(uint32_t, ENDIAN_TO_BIG_32(id));
    MEMPUSH(uint16_t, ENDIAN_TO_BIG_16(count));

    int i;
    for (i = 0; i < count; i++) {

        ndl_sym key = ndl_node_pool_get_key((ndl_node_pool *) graph->pool,
                                            (ndl_ref) node, i);

        ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool,
                                          (ndl_ref) node, key);

        int64_t used = ndl_graph_to_mem_kvpair(graph, key, val, maxlen - curr, to + curr);

        if (used < 0)
            return -1;

        curr += (uint64_t) used;
    }

    return (int64_t) curr;
}

int64_t ndl_graph_to_mem(ndl_graph *graph, uint64_t maxlen, void *mem) {

    char *to = (char *) mem;
    uint64_t curr = 0;

    uint32_t node_count = (uint32_t) ndl_node_pool_size((ndl_node_pool *) graph->pool);
    node_count = ENDIAN_TO_BIG_32(node_count);

    if ((maxlen - curr) < sizeof(node_count))
        return -1;

    MEMPUSH(uint32_t, node_count);

    long int addr = ndl_node_pool_head((ndl_node_pool *) graph->pool);

    while (addr >= 0) {

        int64_t used = ndl_graph_to_mem_node(graph, addr, maxlen - curr, to + curr);

        if (used < 0)
            return -1;

        curr += (uint64_t) used;

        addr = ndl_node_pool_next((ndl_node_pool *) graph->pool, addr);
    }

    return (int64_t) curr;
}
static inline int64_t ndl_graph_from_mem_kv(ndl_graph *graph, ndl_ref node, uint64_t maxlen, char *from) {

    uint64_t curr = 0;

    uint64_t key;
    uint8_t type;
    uint64_t val;

    if ((maxlen - curr) < sizeof(key) + sizeof(type) + sizeof(val))
        return -1;

    MEMPOP(uint64_t, key); key = ENDIAN_FROM_BIG_64(key);
    MEMPOP(uint8_t, type);
    MEMPOP(uint64_t, val); val = ENDIAN_FROM_BIG_64(val);

    ndl_value value;
    value.type = type;
    value.num = (ndl_int) val;

    if (ndl_node_pool_set((ndl_node_pool *) graph->pool, node, key, value) != 0)
        return -1;

    return (int64_t) curr;
}

static inline int64_t ndl_graph_from_mem_node(ndl_graph *graph, uint64_t maxlen, char *from) {

    uint64_t curr = 0;

    uint32_t id;
    uint16_t count;

    if ((maxlen - curr) < sizeof(id) + sizeof(count))
        return -1;

    MEMPOP(uint32_t, id); id = ENDIAN_FROM_BIG_32(id);
    MEMPOP(uint16_t, count); count = (uint16_t) ENDIAN_FROM_BIG_16(count);

    ndl_ref node = ndl_node_pool_alloc_pref((ndl_node_pool *) graph->pool, (ndl_ref) id);

    if (node == NDL_NULL_REF)
        return -1;

    unsigned int i;
    for (i = 0; i < count; i++) {

        int64_t used = ndl_graph_from_mem_kv(graph, node, maxlen - curr, from + curr);
        if (used < 0)
            return -1;

        curr += (uint64_t) used;
    }

    return (int64_t) curr;
}

ndl_graph *ndl_graph_from_mem(uint64_t maxlen, void *mem) {

    ndl_graph *graph = ndl_graph_init();

    if (graph == NULL)
        return NULL;

    char *from = mem;
    uint64_t curr = 0;

    uint32_t count;
    if ((maxlen - curr) < sizeof(count))
        return NULL;

    count = ENDIAN_FROM_BIG_32(*((uint32_t *) &from[curr]));
    curr += sizeof(count);

    unsigned int i;
    for (i = 0; i < count; i++) {

        int64_t used = ndl_graph_from_mem_node(graph, maxlen - curr, from + curr);
        if (used < 0) {
            ndl_graph_kill(graph);
            return NULL;
        }

        curr += (uint64_t) used;
    }

    return graph;
}

int ndl_graph_copy(ndl_graph *to, ndl_graph *from, ndl_ref **refs) {
    return -1;
}

int ndl_graph_dcopy(ndl_graph *to, ndl_graph *from, ndl_ref **roots) {
    return -1;
}

void ndl_graph_print(ndl_graph *graph) {
    ndl_node_pool_print((ndl_node_pool *) graph->pool);
}
