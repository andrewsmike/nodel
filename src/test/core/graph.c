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
