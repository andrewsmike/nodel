#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ndl_test_s {

    const char *path;

    ndl_test_func func;

} ndl_test;

#define NDL_TEST_MAX 1000

static ndl_test ndl_tests[NDL_TEST_MAX];
static int ndl_tests_size = 0;

int ndl_test_register(const char *path, ndl_test_func func) {

    if (ndl_tests_size >= NDL_TEST_MAX)
        return -1;

    ndl_tests[ndl_tests_size].path = path;
    ndl_tests[ndl_tests_size].func = func;

    ndl_tests_size++;

    return 0;
}

static inline int ndl_test_run(int index) {
    printf("[%-28s] Running test.\n", ndl_tests[index].path);

    char *msg = ndl_tests[index].func();

    if (msg == NULL) {
        printf("[%-28s] Success.\n", ndl_tests[index].path);
        return 0;
    } else {
        printf("[%-28s] Failure. Message: '%s'.\n", ndl_tests[index].path, msg);
        return -1;
    }
}

static inline int ndl_test_prefix_match(const char *prefix, const char *path) {

    return !strncmp(prefix, path, strlen(prefix));
}

int ndl_test_irun(const char *prefix) {

    int err = 0;

    int i;
    for (i = 0; i < ndl_tests_size; i++)
        if (ndl_test_prefix_match(prefix, ndl_tests[i].path))
            err |= ndl_test_run(i);

    if (err != 0)
        return -1;
    else
        return 0;
}

/* TODO: Set up an automatic test symbol accounting system. */
static inline void ndl_test_init(void) {

    /* Containers. */
    ndl_test_register("ndl.slab.msize", &ndl_test_slab_msize);
    ndl_test_register("ndl.slab.init", &ndl_test_slab_init);
    ndl_test_register("ndl.slab.minit", &ndl_test_slab_minit);
    ndl_test_register("ndl.slab.it", &ndl_test_slab_it);

    ndl_test_register("ndl.hashtable.alloc", &ndl_test_hashtable_alloc);
    ndl_test_register("ndl.hashtable.minit", &ndl_test_hashtable_minit);
    ndl_test_register("ndl.hashtable.it", &ndl_test_hashtable_it);

    ndl_test_register("ndl.rehashtable.alloc", &ndl_test_rehashtable_alloc);
    ndl_test_register("ndl.rehashtable.minit", &ndl_test_rehashtable_minit);
    ndl_test_register("ndl.rehashtable.it", &ndl_test_rehashtable_it);
    ndl_test_register("ndl.rehashtable.volume", &ndl_test_rehashtable_volume);

    ndl_test_register("ndl.vector.msize", &ndl_test_vector_msize);
    ndl_test_register("ndl.vector.init", &ndl_test_vector_init);
    ndl_test_register("ndl.vector.minit", &ndl_test_vector_minit);
    ndl_test_register("ndl.vector.stack", &ndl_test_vector_stack);
    ndl_test_register("ndl.vector.region", &ndl_test_vector_region);

    ndl_test_register("ndl.heap.msize", &ndl_test_heap_msize);
    ndl_test_register("ndl.heap.init", &ndl_test_heap_init);
    ndl_test_register("ndl.heap.minit", &ndl_test_heap_minit);
    ndl_test_register("ndl.heap.ints", &ndl_test_heap_ints);
    ndl_test_register("ndl.heap.meta", &ndl_test_heap_meta);

    /* Core. */
    ndl_test_register("ndl.node.value.print", &ndl_test_node_value_print);

    ndl_test_register("ndl.graph.alloc", &ndl_test_graph_alloc);
    ndl_test_register("ndl.graph.minit", &ndl_test_graph_minit);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s ndl.test.path.prefix\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ndl_test_init();

    printf("Running tests with prefix '%s.*'.\n", argv[1]);

    return ndl_test_irun(argv[1]);
}
