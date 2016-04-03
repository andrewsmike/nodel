#ifndef NODEL_GRAPH_H
#define NODEL_GRAPH_H

#include "node.h"

typedef ndl_node_pool ndl_graph;


ndl_graph *ndl_graph_init(void);
void       ndl_graph_kill(ndl_graph *graph);

/* Allocate a node.
 * Nodes can be marked as 'root' or not. If marked as root, it will not be GC'd,
 * and all nodes reachable from this node will not be GC'd.
 * If not reachable from a root node, a normal node will be GC'd.
 *
 * alloc() create a root node.
 * free() turn a root node into a normal node.
 * salloc() create a normal node and sets base.key = node.
 * clean() runs the mark-and-sweep GC.
 */
ndl_ref ndl_graph_alloc(ndl_graph *graph);
void    ndl_graph_free(ndl_graph *graph, ndl_ref node);
ndl_ref ndl_graph_salloc(ndl_graph *graph, ndl_ref base, ndl_sym key);
void ndl_graph_clean(ndl_graph *graph);


/* Manipulate key/values.
 * Stores metadata in nodes as hidden metadata keys (beginning with \0.)
 * Allows for garbage collection, backreferences, key indexing.
 *
 * set() set node.key = value, even if node.key does not exist.
 * del() del(node.key).
 * size() get the number of non-hidden keys (including self.)
 * get() get node.key's value.
 * index() get the nth non-hidden key's symbol (including .self, indexing from zero.)
 */
void ndl_graph_set(ndl_graph *graph, ndl_ref node, ndl_sym key, ndl_value value);
void ndl_graph_del(ndl_graph *graph, ndl_ref node, ndl_sym key);
int ndl_graph_size(ndl_graph *graph, ndl_ref node);
ndl_value ndl_graph_get(ndl_graph *graph, ndl_ref node, ndl_sym key);
ndl_sym ndl_graph_index(ndl_graph *graph, ndl_ref node, int index);

/* Get backreferences.
 * Gets the number of backreferences (including .self),
 * and gets each referencing node.
 */
int ndl_graph_backref_count(ndl_graph *graph, ndl_ref node);
ndl_ref ndl_graph_backref_index(ndl_graph *graph, ndl_ref node, int index);

#endif /* NODEL_GRAPH_H */
