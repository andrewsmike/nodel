#ifndef NODEL_GRAPH_H
#define NODEL_GRAPH_H

#include "node.h"

typedef struct ndl_graph_s {

    int64_t sweep;
    uint8_t pool[];
} ndl_graph;

/* Create and destroy a node graph.
 *
 * init() creates a new graph.
 * kill() frees all graph resources and the given graph.
 *
 * minit() creates a graph in the given region.
 * msize() gives the size needed to store a graph.
 * mkill() frees the graph's resources, but not its region.
 */
ndl_graph *ndl_graph_init(void);
void       ndl_graph_kill(ndl_graph *graph);

ndl_graph *ndl_graph_minit(void *region);
void       ndl_graph_mkill(ndl_graph *graph);
uint64_t   ndl_graph_msize(void);

/* Allocate a node.
 * Nodes can be marked as 'root' or not. If marked as root, it will not be GC'd,
 * and all nodes reachable from this node will not be GC'd.
 * If not reachable from a root node, a normal node will be GC'd.
 *
 * alloc() creates a root node.
 * salloc() creates a normal node and sets base.key = node.
 *
 * stat() returns 0 for normal nodes, 1 for root nodes, and -1 on error.
 * mark() turns a node into a root node.
 * unmark() turns a node into a normal node.
 *
 * clean() runs the mark-and-sweep GC.
 */
ndl_ref ndl_graph_alloc (ndl_graph *graph);
ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key);

int ndl_graph_stat  (ndl_graph *graph, ndl_ref node);
int ndl_graph_mark  (ndl_graph *graph, ndl_ref node);
int ndl_graph_unmark(ndl_graph *graph, ndl_ref node);

void ndl_graph_clean(ndl_graph *graph);


/* Manipulate key/values.
 * Allows for garbage collection, backreferences, key indexing, on
 * top of basic key/value get/set/delete.
 *
 * set() sets node.key = value, create node.key if it does not already exist.
 * get() gets node.key's value.
 * del() del(node.key).
 *
 * size() gets the number of non-hidden keys (including self.)
 * index() gets the nth non-hidden key's symbol (including self, indexing from zero.)
 */
int       ndl_graph_set(ndl_graph *graph, ndl_ref node, ndl_sym key, ndl_value value);
ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key);
int       ndl_graph_del(ndl_graph *graph, ndl_ref node, ndl_sym key);

int64_t ndl_graph_size (ndl_graph *graph, ndl_ref node);
ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int64_t index);


/* Search backreferences.
 * Every node that references this node is indexable.
 * If a node references this node multiple times, it only has one backref.
 *
 * size() gets the number of backreferences (including self.)
 * index() gets the nth backreference.
 */
int64_t ndl_graph_backref_size (ndl_graph *graph, ndl_ref node);
ndl_ref ndl_graph_backref_index(ndl_graph *graph, ndl_ref node, int64_t index);


/* Graph serialization.
 * Root/normal property is preserved, as well as addressing.
 * Also copies hidden data, like backrefs and root property (in GC sweep number)
 *
 * mem_est() returns a guess upper bound on memory requirements.
 *     May be too small. Double size until success.
 * to_mem() saves a graph into a given region of memory.
 *     Returns number of bytes used. -1 on error, including overflow.
 * from_mem() retrieves a graph from a block of memory.
 *     Returns NULL on error.
 */
int64_t    ndl_graph_mem_est (ndl_graph *graph);
int64_t    ndl_graph_to_mem  (ndl_graph *graph, uint64_t maxlen, void *mem);
ndl_graph *ndl_graph_from_mem(                  uint64_t maxlen, void *mem);


/* Graph copy operations.
 * Copy operations do not preserve addresses, but they preserve structure
 * and  may preserve root/normal property. Some copy operations only copy
 * a node, and all nodes it can reach, rather than the entire graph.
 *
 * copy() copies the entire graph into another. Preserves root/normal markings.
 *     Returns the number of successfully mapped nodes, and -1 on error.
 *     If a NDL_NULL_REF terminated array of references is given, they will
 *     be replaced by their new counterparts.
 * dcopy() copies all nodes reachable from the given NULL terminated array
 *     of root nodes. All nodes are marked as normal, and the roots as roots.
 *     Returns the number of copied roots on success, -1 on error.
 *     Writes the new addresses of the root nodes to the given array.
 */
int ndl_graph_copy (ndl_graph *to, ndl_graph *from, ndl_ref *refs);
int ndl_graph_dcopy(ndl_graph *to, ndl_graph *from, ndl_ref *roots);

/* Print the entirety of a graph. */
void ndl_graph_print(ndl_graph *graph);

#endif /* NODEL_GRAPH_H */
