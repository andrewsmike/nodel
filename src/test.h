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
char *ndl_test_slab_msize(void);
char *ndl_test_slab_init(void);
char *ndl_test_slab_minit(void);
char *ndl_test_slab_it(void);

char *ndl_test_node_value_print(void);

char *ndl_test_hashtable_alloc(void);
char *ndl_test_hashtable_minit(void);
char *ndl_test_hashtable_it(void);

char *ndl_test_rehashtable_alloc(void);
char *ndl_test_rehashtable_minit(void);
char *ndl_test_rehashtable_it(void);
char *ndl_test_rehashtable_volume(void);


#endif /* NODEL_TEST_H */
