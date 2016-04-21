#include "test.h"

#include "graph.h"

char *ndl_test_graph_alloc(void) {

    ndl_graph *graph = ndl_graph_init();
    if (graph == NULL)
        return "Failed to allocate graph";

    ndl_graph_kill(graph);

    return NULL;
}

char *ndl_test_graph_minit(void) {

    void *region = malloc(ndl_graph_msize());
    if (region == NULL)
        return "Failed to allocate region, couldn't run test";

    ndl_graph *graph = ndl_graph_minit(region);
    if (graph == NULL) {
        free(region);
        return "Couldn't do in-place initialization of graph";
    }

    if (region != graph) {
        free(region);
        return "Messes with the pointer";
    }

    ndl_graph_mkill(graph);

    free(region);

    return NULL;
}

char *ndl_test_graph_salloc(void) {

    ndl_graph *graph = ndl_graph_init();
    if (graph == NULL)
        return "Failed to allocate graph";

    ndl_ref a = ndl_graph_alloc(graph);
    ndl_ref b = ndl_graph_alloc(graph);
    ndl_ref c = ndl_graph_alloc(graph);
    if ((a == NDL_NULL_REF) || (b == NDL_NULL_REF) || (c == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to allocate nodes";
    }

    ndl_ref aa = ndl_graph_salloc(graph,  a, NDL_SYM("next    "));
    ndl_ref ab = ndl_graph_salloc(graph, aa, NDL_SYM("next    "));
    ndl_ref ac = ndl_graph_salloc(graph, ab, NDL_SYM("next    "));
    ndl_ref ba = ndl_graph_salloc(graph,  b, NDL_SYM("next    "));
    if ((aa == NDL_NULL_REF) || (ab == NDL_NULL_REF) ||
        (ac == NDL_NULL_REF) || (ba == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to sallocate nodes";
    }

    int amark = ndl_graph_stat(graph, a);
    int bmark = ndl_graph_stat(graph, b);
    int aamark = ndl_graph_stat(graph, aa);
    int acmark = ndl_graph_stat(graph, aa);
    if ((amark != 1) || (bmark != 1) ||
        (aamark != 0) || (acmark != 0)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Bad root/normal values";
    }

    int err = ndl_graph_unmark(graph, b);
    bmark = ndl_graph_stat(graph, b);
    if ((err != 0) || (bmark != 0)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to unmark node";
    }

    err = ndl_graph_mark(graph, ba);
    int bamark = ndl_graph_stat(graph, ba);
    if ((err != 0) || (bamark != 1)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to unmark node";
    }

    ndl_graph_kill(graph);

    return NULL;
}

char *ndl_test_graph_gc(void) {

    ndl_graph *graph = ndl_graph_init();
    if (graph == NULL)
        return "Failed to allocate graph";

    ndl_ref a = ndl_graph_alloc(graph);
    ndl_ref b = ndl_graph_alloc(graph);
    if ((a == NDL_NULL_REF) || (b == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to allocate nodes";
    }

    ndl_ref aa = ndl_graph_salloc(graph,  a, NDL_SYM("next    "));
    ndl_ref ab = ndl_graph_salloc(graph, aa, NDL_SYM("next    "));
    ndl_ref ac = ndl_graph_salloc(graph, ab, NDL_SYM("next    "));
    ndl_ref ba = ndl_graph_salloc(graph,  b, NDL_SYM("next    "));
    if ((aa == NDL_NULL_REF) || (ab == NDL_NULL_REF) ||
        (ac == NDL_NULL_REF) || (ba == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to sallocate nodes";
    }

    int err = ndl_graph_del(graph, b, NDL_SYM("next    "));
    if (err != 0) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to delete kvpair";
    }

    ndl_graph_clean(graph);

    err = ndl_graph_stat(graph, ba);
    if (err != -1) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to clean node";
    }

    err = ndl_graph_set(graph, ac, NDL_SYM("next    "), NDL_VALUE(EVAL_REF, ref=aa));
    err = ndl_graph_unmark(graph, a);
    if (err != 0) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to add reference or unmark root node";
    }

    ndl_graph_clean(graph);
    err = ndl_graph_stat(graph, a);
    if (err != -1) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to clean node";
    }

    err = ndl_graph_stat(graph, ac);
    if (err != -1) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to clean node";
    }

    ndl_graph_kill(graph);

    return NULL;
}

char *ndl_test_graph_kv_it(void) {

    return NULL;
}

char *ndl_test_graph_backref(void) {

    ndl_graph *graph = ndl_graph_init();
    if (graph == NULL)
        return "Failed to allocate graph";

    ndl_ref a = ndl_graph_alloc(graph);
    ndl_ref b = ndl_graph_alloc(graph);
    if ((a == NDL_NULL_REF) || (b == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to allocate nodes";
    }

    ndl_ref aa = ndl_graph_salloc(graph,  a, NDL_SYM("next    "));
    ndl_ref ab = ndl_graph_salloc(graph, aa, NDL_SYM("next    "));
    ndl_ref ac = ndl_graph_salloc(graph, ab, NDL_SYM("next    "));
    ndl_ref ba = ndl_graph_salloc(graph,  b, NDL_SYM("next    "));
    if ((aa == NDL_NULL_REF) || (ab == NDL_NULL_REF) ||
        (ac == NDL_NULL_REF) || (ba == NDL_NULL_REF)) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to sallocate nodes";
    }

    int err = ndl_graph_set(graph, ac, NDL_SYM("next    "), NDL_VALUE(EVAL_REF, ref=aa));
    err |= ndl_graph_set(graph, ac, NDL_SYM("next1   "), NDL_VALUE(EVAL_REF, ref=aa));
    err |= ndl_graph_set(graph, ac, NDL_SYM("next2   "), NDL_VALUE(EVAL_REF, ref=aa));
    err |= ndl_graph_set(graph, ac, NDL_SYM("next1   "), NDL_VALUE(EVAL_REF, ref=ab));
    err |= ndl_graph_set(graph, aa, NDL_SYM("back    "), NDL_VALUE(EVAL_REF, ref=ac));
    if (err != 0) {
        ndl_graph_print(graph);
        ndl_graph_kill(graph);
        return "Failed to add references";
    }

    ndl_graph_kill(graph);

    return NULL;
}

char *ndl_test_graph_mem(void) {

    printf("TODO: Graph serialization tests.\n");
    return "Test not yet written";
}

char *ndl_test_graph_copy(void) {

    printf("TODO: Graph copy tests.\n");
    return "Test not yet written";
}
