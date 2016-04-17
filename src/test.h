/* src/test/test.h: Test registry and prototypes.
 */
#ifndef NODEL_TEST_H
#define NODEL_TEST_H

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
char *ndl_test_slab_init(void);
char *ndl_test_slab_kill(void);


#endif /* NODEL_TEST_H */
