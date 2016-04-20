#include "test.h"

#include "heap.h"

#include <stdio.h>
#include <string.h>

static int ndl_test_heap_cmp_func(void *a, void *b) {

    int ai = *((int *) a);
    int bi = *((int *) b);

    if (ai < bi)
        return -1;
    else if (ai > bi)
        return 1;
    else
        return 0;
}

/* Size is invariant to elem_count. */
char *ndl_test_heap_msize(void) {

    uint64_t size = ndl_heap_msize(0, &ndl_test_heap_cmp_func);
    if (ndl_heap_msize(100, &ndl_test_heap_cmp_func) != size)
        return "Gave different sizes";
    else if (ndl_heap_msize(100000, &ndl_test_heap_cmp_func) != size)
        return "Gave different sizes";

    return NULL;
}

char *ndl_test_heap_init(void) {

    ndl_heap *heap = ndl_heap_init(sizeof(int), &ndl_test_heap_cmp_func);
    if (heap == NULL)
        return "Failed to allocate";

    ndl_heap_kill(heap);

    return NULL;
}

char *ndl_test_heap_minit(void) {

    void *region = malloc(ndl_heap_msize(sizeof(int), &ndl_test_heap_cmp_func));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_heap *heap = ndl_heap_minit(region, sizeof(int), &ndl_test_heap_cmp_func);
    if (heap == NULL) {
        free(region);
        return "In-place initialization failed";
    }

    if (heap != region) {
        ndl_heap_mkill(heap);
        free(region);
        return "Messes with the pointer";
    }

    ndl_heap_mkill(heap);
    free(region);

    return NULL;
}

