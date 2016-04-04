#ifndef NODEL_GRAPH_H
#define NODEL_GRAPH_H

#include "node.h"

typedef struct ndl_graph_s ndl_graph;

/* Create and destroy a node graph.
 *
 * init() creates a new graph.
 * kill() frees all graph resources and the given graph.
 */
ndl_graph *ndl_graph_init(void);
void       ndl_graph_kill(ndl_graph *graph);


/* Allocate a node.
 * Nodes can be marked as 'root' or not. If marked as root, it will not be GC'd,
 * and all nodes reachable from this node will not be GC'd.
 * If not reachable from a root node, a normal node will be GC'd.
 *
 * alloc() create a root node.
 * salloc() create a normal node and sets base.key = node.
 *
 * mark() turns a node into a root node.
 * unmark() turn a node into a normal node.
 *
 * clean() runs the mark-and-sweep GC.
 */
ndl_ref ndl_graph_alloc (ndl_graph *graph);
ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key);

int ndl_graph_mark  (ndl_graph *graph, ndl_ref node);
int ndl_graph_unmark(ndl_graph *graph, ndl_ref node);

void ndl_graph_clean(ndl_graph *graph);


/* Manipulate key/values.
 * Allows for garbage collection, backreferences, key indexing, on
 * top of basic key/value get/set/delete.
 *
 * set() set node.key = value, create node.key if it does not already exist.
 * get() get node.key's value.
 * del() del(node.key).
 *
 * size() get the number of non-hidden keys (including self.)
 * index() get the nth non-hidden key's symbol (including self, indexing from zero.)
 */
int       ndl_graph_set(ndl_graph *graph, ndl_ref node, ndl_sym key, ndl_value value);
ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key);
int       ndl_graph_del(ndl_graph *graph, ndl_ref node, ndl_sym key);

int     ndl_graph_size (ndl_graph *graph, ndl_ref node);
ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int index);


/* Search backreferences.
 * Every node that references this node is indexable.
 * If a node references this node multiple times, it only has one backref.
 *
 * size() get the number of backreferences (including self.)
 * index() get the nth backreference.
 */
int     ndl_graph_backref_size (ndl_graph *graph, ndl_ref node);
ndl_ref ndl_graph_backref_index(ndl_graph *graph, ndl_ref node, int index);

#endif /* NODEL_GRAPH_H */
