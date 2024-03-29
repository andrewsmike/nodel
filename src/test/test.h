/* src/test/test.h: Test registry and prototypes.
 */
#ifndef NODEL_TEST_H
#define NODEL_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Register testing functions under a path.
 * Functions return NULL on success, error string on error.
 * Path separators are arbitrary, though nodel uses
 * 'ndl.graph.salloc.test1', for example.
 */
typedef char *(*ndl_test_func)(void);

/* Register functions and run all tests
 * under a prefix. Continues on error.
 * Prints interactively to the console.
 * Returns 0 on success, -1 on error.
 */
int ndl_test_register(const char *path, ndl_test_func func);
int ndl_test_irun(const char *prefix);

/* TODO: Set up an automatic test symbol accounting system. */

/* Containers. */
char *ndl_test_slab_msize(void);
char *ndl_test_slab_init(void);
char *ndl_test_slab_minit(void);
char *ndl_test_slab_it(void);

char *ndl_test_hashtable_alloc(void);
char *ndl_test_hashtable_minit(void);
char *ndl_test_hashtable_it(void);

char *ndl_test_rehashtable_alloc(void);
char *ndl_test_rehashtable_minit(void);
char *ndl_test_rehashtable_it(void);
char *ndl_test_rehashtable_volume(void);

char *ndl_test_vector_msize(void);
char *ndl_test_vector_init(void);
char *ndl_test_vector_minit(void);
char *ndl_test_vector_stack(void);
char *ndl_test_vector_region(void);

char *ndl_test_heap_msize(void);
char *ndl_test_heap_init(void);
char *ndl_test_heap_minit(void);
char *ndl_test_heap_ints(void);
char *ndl_test_heap_meta(void);


/* Core */
char *ndl_test_node_value_print(void);

char *ndl_test_asm_syntax(void);

char *ndl_test_graph_alloc(void);
char *ndl_test_graph_minit(void);
char *ndl_test_graph_salloc(void);
char *ndl_test_graph_gc(void);
char *ndl_test_graph_kv_it(void);
char *ndl_test_graph_backref(void);

/* Runtime */
char *ndl_test_time_conv(void);
char *ndl_test_time_add(void);
char *ndl_test_time_get(void);

#endif /* NODEL_TEST_H */
