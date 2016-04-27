#include "graph.h"
#include "nodepool.h"
#include "ndlendian.h"
#include "rehashtable.h"

#include <stdlib.h>
#include <stdio.h>

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

    int err = ndl_node_pool_put((ndl_node_pool *) graph->pool,
                                ret, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-1));

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

    return ndl_node_pool_put((ndl_node_pool *) graph->pool,
                             node, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=0));
}

int ndl_graph_mark(ndl_graph *graph, ndl_ref node) {

    return ndl_node_pool_put((ndl_node_pool *) graph->pool,
                             node, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-1));
}

ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key) {

    ndl_ref ret = ndl_node_pool_alloc((ndl_node_pool *) graph->pool);

    if (ret == NDL_NULL_REF)
        return ret;

    int err = ndl_node_pool_put((ndl_node_pool *) graph->pool,
                                ret, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=0));

    err |= ndl_node_pool_put((ndl_node_pool *) graph->pool,
                             ret, NDL_BACKREF(base), NDL_VALUE(EVAL_INT, ref=1));

    if (err == 0)
        err |= ndl_node_pool_put((ndl_node_pool *) graph->pool,
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

    if (gcsweep.num == -1 && (sweep >= 0))
        return;

    sweep = (sweep < 0)? -sweep : sweep; 

    if (gcsweep.num >= sweep)
        return;

    if (gcsweep.num >= 0) {

        gcsweep.num = sweep;

        ndl_node_pool_put((ndl_node_pool *) graph->pool,
                          root, NDL_SYM("\0gcsweep"), gcsweep);
    }

    void *curr = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, root);

    while (curr != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, root, curr);

        if (!NDL_ISHIDDEN(key)) {
            ndl_value next = ndl_node_pool_get((ndl_node_pool *) graph->pool, root, key);
            if (next.type == EVAL_REF && next.ref != NDL_NULL_REF)
                ndl_graph_clean_mark(graph, next.ref, sweep);
        }

        curr = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, root, curr);
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

    return ndl_node_pool_put(pool, from, NDL_BACKREF(to), value);
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

    return ndl_node_pool_put(pool, from, NDL_BACKREF(to), value);
}

static void ndl_graph_clean_remove(ndl_graph *graph, ndl_ref node) {

    void *curr = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, node);

    while (curr != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, node, curr);

        if (!NDL_ISHIDDEN(key)) {
            ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool, node, key);
            if ((val.type == EVAL_REF) && (val.ref != NDL_NULL_REF))
                ndl_graph_rm_backref((ndl_node_pool *) graph->pool, val.ref, node);
        }

        curr = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, node, curr);
    }

    ndl_node_pool_free((ndl_node_pool *) graph->pool, node);
}

void ndl_graph_clean(ndl_graph *graph) {

    int64_t sweep = ++graph->sweep;

    ndl_node_pool *pool = (ndl_node_pool *) graph->pool;

    void *curr = ndl_node_pool_head(pool);
    while (curr != NULL) {

        ndl_ref key = ndl_node_pool_node(pool, curr);
        if (key == NDL_NULL_REF)
            break;

        ndl_value gcsweep = ndl_node_pool_get(pool, key, NDL_SYM("\0gcsweep"));

        if ((gcsweep.type == EVAL_INT) && (gcsweep.num == -1))
            ndl_graph_clean_mark(graph, key, -sweep);

        curr = ndl_node_pool_next(pool, curr);
    }

    curr = ndl_node_pool_head(pool);
    while (curr != NULL) {

        ndl_ref key = ndl_node_pool_node(pool, curr);
        if (key == NDL_NULL_REF)
            break;

        ndl_value gcsweep = ndl_node_pool_get(pool, key, NDL_SYM("\0gcsweep"));

        if (!((gcsweep.type == EVAL_INT) &&
              ((gcsweep.num == -1) || (gcsweep.num >= sweep))))
            ndl_graph_clean_remove(graph, key);

        curr = ndl_node_pool_next(pool, curr);
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

    err = ndl_node_pool_put((ndl_node_pool *) graph->pool,
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

    void *curr = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, node);

    int sum = 0;

    while (curr != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, node, curr);
        if (!NDL_ISHIDDEN(key))
            sum++;

        curr = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, node, curr);
    }

    return sum;
}

ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key) {

    return ndl_node_pool_get((ndl_node_pool *) graph->pool,
                             node, key);
}

ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int64_t index) {

    void *curr = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, node);

    int sum = 0;

    while (curr != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, node, curr);

        if (!NDL_ISHIDDEN(key)) {
            if (sum == index)
                return key;
            sum++;
        }

        curr = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, node, curr);
    }

    return NDL_NULL_SYM;
}

static inline void *ndl_graph_backref_next_back(ndl_graph *graph, ndl_ref node, void *curr) {

    ndl_node_pool *pool = (ndl_node_pool *) graph->pool;
    while (curr != NULL) {
        ndl_sym key = ndl_node_pool_node_pairs_key(pool, node, curr);
        if (NDL_ISBACKREF(key))
            return curr;

        curr = ndl_node_pool_node_pairs_next(pool, node, curr);
    }

    return NULL;
}

void *ndl_graph_backref_head(ndl_graph *graph, ndl_ref node) {

    void *curr = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, node);

    return ndl_graph_backref_next_back(graph, node, curr);
}

void *ndl_graph_backref_next(ndl_graph *graph, ndl_ref node, void *prev) {

    prev = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, node, prev);

    return ndl_graph_backref_next_back(graph, node, prev);
}

ndl_ref ndl_graph_backref_node(ndl_graph *graph, ndl_ref node, void *curr) {

    ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, node, curr);
    if ((key == NDL_NULL_SYM) || (!NDL_ISBACKREF(key)))
        return NDL_NULL_REF;
    else
        return NDL_DEBACKREF(key);
}

uint64_t ndl_graph_backref_count(ndl_graph *graph, ndl_ref node, void *curr) {

    ndl_value val = ndl_node_pool_node_pairs_val((ndl_node_pool *) graph->pool, node, curr);
    if (val.type != EVAL_INT)
        return 0;
    else
        return (uint64_t) val.num;
}

uint64_t ndl_graph_backrefs(ndl_graph *graph, ndl_ref to, ndl_ref from) {

    ndl_value val = ndl_node_pool_get((ndl_node_pool *) graph->pool, to, NDL_BACKREF(from));
    if (val.type != EVAL_INT)
        return 0;
    else
        return (uint64_t) val.num;
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

uint64_t ndl_graph_mem_est(ndl_graph *graph) {

    uint64_t nodes = ndl_node_pool_size((ndl_node_pool *) graph->pool);

    /* Assume there are not more than 16 keys per node.
     * If there are, caller functions will just retry with more.
     * If there are not, it's not an unreasonable amount of wasted memory.
     */
    uint64_t estkvpairs = nodes * 16;

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

    uint16_t count = (uint16_t) ndl_node_pool_node_size((ndl_node_pool *) graph->pool, (ndl_ref) node);

    if ((maxlen - curr) < (sizeof(id) + sizeof(count)))
        return -1;

    MEMPUSH(uint32_t, ENDIAN_TO_BIG_32(id));
    MEMPUSH(uint16_t, ENDIAN_TO_BIG_16(count));

    void *currkv = ndl_node_pool_node_pairs_head((ndl_node_pool *) graph->pool, node);

    while (currkv != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key((ndl_node_pool *) graph->pool, node, currkv);

        ndl_value val = ndl_node_pool_node_pairs_val((ndl_node_pool *) graph->pool, (ndl_ref) node, currkv);

        int64_t used = ndl_graph_to_mem_kvpair(graph, key, val, maxlen - curr, to + curr);

        if (used < 0)
            return -1;

        curr += (uint64_t) used;

        currkv = ndl_node_pool_node_pairs_next((ndl_node_pool *) graph->pool, node, currkv);
    }

    return (int64_t) curr;
}

int64_t ndl_graph_to_mem(ndl_graph *graph, uint64_t maxlen, void *mem) {

    char *to = (char *) mem;
    uint64_t curr = 0;
    ndl_node_pool *pool = (ndl_node_pool *) graph->pool;

    uint32_t node_count = (uint32_t) ndl_node_pool_size(pool);
    node_count = ENDIAN_TO_BIG_32(node_count);

    if ((maxlen - curr) < sizeof(node_count))
        return -1;

    MEMPUSH(uint32_t, node_count);

    void *currnode = ndl_node_pool_head(pool);

    while (currnode != NULL) {

        ndl_ref node = ndl_node_pool_node(pool, currnode);
        if (node == NDL_NULL_REF)
            break;

        int64_t used = ndl_graph_to_mem_node(graph, node, maxlen - curr, to + curr);

        if (used < 0)
            return -1;

        curr += (uint64_t) used;

        currnode = ndl_node_pool_next(pool, currnode);
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

    if (ndl_node_pool_put((ndl_node_pool *) graph->pool, node, key, value) != 0)
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

static inline ndl_ref ndl_graph_copy_node(ndl_node_pool *to, ndl_node_pool *from, ndl_ref node) {

    ndl_ref new = ndl_node_pool_alloc(to);
    if (new == NDL_NULL_REF)
        return NDL_NULL_REF;

    void *kvpair = ndl_node_pool_node_pairs_head(from, node);
    while (kvpair != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key(from, node, kvpair);
        ndl_value val = ndl_node_pool_node_pairs_val(from, node, kvpair);
        if (key == NDL_SYM("\0gcsweep")) {
            if ((val.type == EVAL_INT) && (val.num == -1)) {
                val.num = -2;
            }
        }

        int err = ndl_node_pool_put(to, new, key, val);
        if (err != 0) {
            ndl_node_pool_free(to, new);
            return NDL_NULL_REF;
        }

        kvpair = ndl_node_pool_node_pairs_next(from, node, kvpair);
    }

    return new;
}
static inline int ndl_graph_copy_fix_node(ndl_node_pool *to, ndl_rhashtable *mapping, ndl_ref old) {

    ndl_ref *new = ndl_rhashtable_get(mapping, &old);
    if (new == NULL)
        return -1;

    void *kvpair = ndl_node_pool_node_pairs_head(to, *new);
    while (kvpair != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key(to, *new, kvpair);
        if (key == NDL_NULL_SYM)
            return -1;

        ndl_value val = ndl_node_pool_node_pairs_val(to, *new, kvpair);

        if (NDL_ISBACKREF(key)) {

            ndl_ref ref = NDL_DEBACKREF(key);
            ndl_ref *nref = ndl_rhashtable_get(mapping, &ref);
            if (nref == NULL)
                return -1;

            int err = ndl_node_pool_put(to, *new, NDL_BACKREF(nref), val);
            if (err != 0)
                return -1;

            err = ndl_node_pool_del(to, *new, key);
            if (err != 0)
                return -1;

        } else if (key == NDL_SYM("\0gcsweep")) {

            if (val.type != EVAL_INT)
                return -1;

            if (val.num >= 0)
                val.num = 0;
            else
                val.num = -1;

            int err =  ndl_node_pool_put(to, *new, key, val);
            if (err != 0)
                return -1;
        }

        kvpair = ndl_node_pool_node_pairs_next(to, *new, kvpair);
    }

    return 0;
}

int ndl_graph_copy(ndl_graph *to, ndl_graph *from, ndl_ref *refs) {

    /* Create mapping table.
     * Iterate over nodes, insert with bad backrefs
     * Iterate over nodes, correct backrefs, reset sweep (mark)
     * Update refs
     * Return
     */
    ndl_rhashtable *mapping = ndl_rhashtable_init(sizeof(ndl_ref), sizeof(ndl_ref), 16);
    if (mapping == NULL)
        return -1;

    ndl_node_pool *pool = (ndl_node_pool *) from->pool;

    void *curr = ndl_node_pool_head(pool);
    while (curr != NULL) {

        ndl_ref old = ndl_node_pool_node(pool, curr);
        if (old == NDL_NULL_REF)
            break;

        ndl_ref new = ndl_graph_copy_node((ndl_node_pool *) to->pool, pool, old);
        void *mapping = ndl_rhashtable_put(mapping, &old, &new);
        if ((new == NDL_NULL_REF) || (mapping == NULL)) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }

        curr = ndl_node_pool_next(pool, curr);
    }

    curr = ndl_node_pool_head(pool);
    while (curr != NULL) {

        ndl_ref old = ndl_node_pool_node(pool, curr);
        if (old == NDL_NULL_REF)
            break;

        int err = ndl_graph_copy_fix_node((ndl_node_pool *) to->pool, mapping, old);
        if (err != 0) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }

        curr = ndl_node_pool_next(pool, curr);
    }

    if (refs != NULL) {

        for (; *refs != NDL_NULL_REF; refs++) {

            ndl_ref *res = ndl_rhashtable_get(mapping, refs);
            if (res == NULL) {
                ndl_rhashtable_kill(mapping);
                return -1;
            }

            *refs = *res;
        }
    }

    ndl_rhashtable_kill(mapping);

    return 0;
}

static inline int ndl_graph_dcopy_add(ndl_node_pool *to, ndl_node_pool *from,
                                      ndl_ref node, ndl_rhashtable *mapping) {

    /* If in mapping, return
     * create a new node,
     * set gc=-3
     * add mapping,
     * for each kv pair:
     *   if ref, recurse
     * return
     */
    void *ret = ndl_rhashtable_get(mapping, &node);
    if (ret != NULL)
        return 0;

    ndl_ref new = ndl_node_pool_alloc(to);
    if (new == NDL_NULL_REF)
        return -1;

    ret = ndl_rhashtable_put(mapping, &node, &new);
    if (ret == NULL) {
        ndl_node_pool_free(to, new);
        return -1;
    }

    int err = ndl_node_pool_put(to, new, NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-3));
    if (err != 0) {
        ndl_node_pool_free(to, new);
        return -1;
    }

    void *curr = ndl_node_pool_node_pairs_head(from, node);
    while (curr != NULL) {

        ndl_value val = ndl_node_pool_node_pairs_val(from, node, curr);

        if ((val.type == EVAL_REF) && (val.ref != NDL_NULL_REF)) {

            err = ndl_graph_dcopy_add(to, from, val.ref, mapping);
            if (err != 0) {
                ndl_node_pool_free(to, new);
                return -1;
            }
        }

        curr = ndl_node_pool_node_pairs_next(from, node, curr);
    }

    return 0;
}

static inline int ndl_graph_dcopy_fix(ndl_node_pool *to, ndl_node_pool *from,
                                      ndl_ref node, ndl_rhashtable *mapping) {

    /*  
     * if GC != -3, return,
     * set GC to 0
     * for each (origin) kv pair:
     *    if GC, ignore
     *    if backref, if remappable, remap, copy
     *    if ref, remap, recurse
     *    else, copy
     * return
     */

    ndl_ref *new = ndl_rhashtable_get(mapping, &node);
    if (new == NULL)
        return -1;

    ndl_value gc = ndl_node_pool_get(to, *new, NDL_SYM("\0gcsweep"));
    if ((gc.type != EVAL_INT) || (gc.num != -3))
        return 0;

    gc.num = 0;
    int err = ndl_node_pool_put(to, *new, NDL_SYM("\0gcsweep"), gc);
    if (err != 0)
        return -1;

    void *curr = ndl_node_pool_node_pairs_head(from, node);
    while (curr != NULL) {

        ndl_sym key = ndl_node_pool_node_pairs_key(from, node, curr);
        ndl_value val = ndl_node_pool_node_pairs_val(from, node, curr);

        if (key == NDL_SYM("\0gcsweep")) {
            key = NDL_NULL_SYM;
        } else if (NDL_ISBACKREF(key)) {

            ndl_ref old_ref = NDL_DEBACKREF(key);
            ndl_ref *new_ref = ndl_rhashtable_get(mapping, &old_ref);
            if (new_ref == NULL)
                key = NDL_NULL_SYM;
            else
                key = NDL_BACKREF(*new_ref);

        } else if ((val.type == EVAL_REF) && (val.ref != NDL_NULL_REF)) {

            err = ndl_graph_dcopy_fix(to, from, val.ref, mapping);
            if (err != 0)
                return -1;

            ndl_ref *new_ref = ndl_rhashtable_get(mapping, &val.ref);
            if (new_ref == NULL)
                return -1;

            val.ref = *new_ref;
        }

        if (key != NDL_NULL_SYM) {
            err = ndl_node_pool_put(to, *new, key, val);
            if (err != 0)
                return -1;
        }

        curr = ndl_node_pool_node_pairs_next(from, node, curr);
    }

    return 0;
}

int ndl_graph_dcopy(ndl_graph *to, ndl_graph *from, ndl_ref *roots) {

    /* Create mapping table,
     * for each root:
     *    recursively add root and reachable to table with bad refs and bad GC
     * for each root:
     *    recursively correct references and GC
     *    mark root
     *    map root arg
     * return
     */
    ndl_rhashtable *mapping = ndl_rhashtable_init(sizeof(ndl_ref), sizeof(ndl_ref), 16);
    if (mapping == NULL)
        return -1;

    ndl_node_pool *srcpool = (ndl_node_pool *) from->pool;
    ndl_node_pool *dstpool = (ndl_node_pool *) to->pool;

    ndl_ref *base;
    for (base = roots; *base != NDL_NULL_REF; base++) {

        int err = ndl_graph_dcopy_add(dstpool, srcpool, *base, mapping);
        if (err != 0) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }
    }

    for (base = roots; *base != NDL_NULL_REF; base++) {

        ndl_ref *new = ndl_rhashtable_get(mapping, base);
        if (new == NULL) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }

        int err = ndl_graph_dcopy_fix(dstpool, srcpool, *base, mapping);
        if (err != 0) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }

        err = ndl_node_pool_put(dstpool, *new,
                                NDL_SYM("\0gcsweep"), NDL_VALUE(EVAL_INT, num=-1));
        if (err != 0) {
            ndl_rhashtable_kill(mapping);
            return -1;
        }

        *base = *new;
    }

    ndl_rhashtable_kill(mapping);

    return 0;
}

void ndl_graph_print(ndl_graph *graph) {
    printf("Printing graph.\n");
    printf("Sweep: %ld.\n", graph->sweep);
    ndl_node_pool_print((ndl_node_pool *) graph->pool);
}
