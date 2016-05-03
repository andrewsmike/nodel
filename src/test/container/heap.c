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

static void ndl_test_heap_swap_func(void *a, void *b) {

    int *ai = ((int *) a);
    int *bi = ((int *) b);
    int ci;

     ci = *ai;
    *ai = *bi;
    *bi =  ci;

    return;
}

/* Size is invariant to elem_count. */
char *ndl_test_heap_msize(void) {

    uint64_t size = ndl_heap_msize(0, &ndl_test_heap_cmp_func,
                                   &ndl_test_heap_swap_func);
    if (ndl_heap_msize(100, &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func) != size)
        return "Gave different sizes";
    else if (ndl_heap_msize(100000, &ndl_test_heap_cmp_func,
                            &ndl_test_heap_swap_func) != size)
        return "Gave different sizes";

    return NULL;
}

char *ndl_test_heap_init(void) {

    ndl_heap *heap = ndl_heap_init(sizeof(int), &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func);
    if (heap == NULL)
        return "Failed to allocate";

    ndl_heap_kill(heap);

    return NULL;
}

char *ndl_test_heap_minit(void) {

    void *region = malloc(ndl_heap_msize(sizeof(int), &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_heap *heap = ndl_heap_minit(region, sizeof(int), &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func);
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

#define HEAPFAIL(cond, msg) if (cond) {ndl_heap_kill(heap); return msg;}
#define HEAPPUT(var, number)                    \
    do {                                        \
        var = number;                           \
        ret = ndl_heap_put(heap, &var);         \
        HEAPFAIL(ret == NULL, "Failed to put"); \
    } while (0)

char *ndl_test_heap_ints(void) {

    ndl_heap *heap = ndl_heap_init(sizeof(int), &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func);
    if (heap == NULL)
        return "Failed to allocate";

    void *ret;
    int a;
    HEAPPUT(a,  0);
    HEAPPUT(a,  7);
    HEAPPUT(a, 14);
    HEAPPUT(a,  2);
    HEAPPUT(a, -1);

    int *head = (int *) ndl_heap_peek(heap);
    HEAPFAIL(*head != 14, "Heap ordered wrong");

    int err = ndl_heap_pop(heap);
    HEAPFAIL(err != 0, "Failed to delete head");

    head = (int *) ndl_heap_peek(heap);
    HEAPFAIL(*head != 7, "Heap ordered wrong");

    *head = -10;
    ndl_heap_readj(heap, head);
    head = (int *) ndl_heap_peek(heap);
    HEAPFAIL(*head != 2, "Heap ordered wrong");
    

    ndl_heap_kill(heap);

    return NULL;
}

char *ndl_test_heap_meta(void) {

    ndl_heap *heap = ndl_heap_init(sizeof(int), &ndl_test_heap_cmp_func,
                       &ndl_test_heap_swap_func);
    if (heap == NULL)
        return "Failed to allocate";

    int a;
    a = 10;
    ndl_heap_put(heap, &a); a = 7;
    ndl_heap_put(heap, &a); a = 14;
    ndl_heap_put(heap, &a); a = 2;
    ndl_heap_put(heap, &a); a = -1;
    ndl_heap_put(heap, &a);

    if (ndl_heap_size(heap) != 5) {
        ndl_heap_kill(heap);
        return "Failed to put elements";
    }

    if (ndl_heap_cap(heap) < 5) {
        ndl_heap_kill(heap);
        return "Invalid capacity";
    }

    if (ndl_heap_data_size(heap) != sizeof(int)) {
        ndl_heap_kill(heap);
        return "Bad data size";
    }

    if (ndl_heap_compare(heap) != ndl_test_heap_cmp_func) {
        ndl_heap_kill(heap);
        return "Bad compare";
    }

    if (ndl_heap_swap(heap) != &ndl_test_heap_swap_func) {
        ndl_heap_kill(heap);
        return "Bad swap";
    }

    ndl_heap_kill(heap);

    return NULL;
}
