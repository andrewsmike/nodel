#include "heap.h"

#include "vector.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <stdio.h>

ndl_heap *ndl_heap_init(uint64_t data_size,
                        ndl_heap_cmp_func compare, ndl_heap_swap_func swap) {

    void *region = malloc(ndl_heap_msize(data_size, compare, swap));
    if (region == NULL)
        return NULL;

    ndl_heap *ret = ndl_heap_minit(region, data_size, compare, swap);
    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_heap_kill(ndl_heap *heap) {

    if (heap == NULL)
        return;

    ndl_heap_mkill(heap);

    free(heap);
}

ndl_heap *ndl_heap_minit(void *region, uint64_t data_size,
                         ndl_heap_cmp_func compare, ndl_heap_swap_func swap) {

    ndl_heap *heap = (ndl_heap *) region;
    if (heap == NULL)
        return NULL;

    ndl_vector *ret = ndl_vector_minit((ndl_vector *) &heap->vector, data_size);
    if (ret == NULL)
        return NULL;

    heap->compare = compare;
    heap->swap = swap;

    return heap;
}

void ndl_heap_mkill(ndl_heap *heap) {

    if (heap == NULL)
        return;

    ndl_vector_mkill((ndl_vector *) &heap->vector);

    return;
}

uint64_t ndl_heap_msize(uint64_t data_size,
                        ndl_heap_cmp_func compare, ndl_heap_swap_func swap) {

    return sizeof(ndl_heap) + ndl_vector_msize(data_size);
}

static inline void *ndl_heap_parent(ndl_vector *vec, void *elem) {

    void *base = ndl_vector_get(vec, 0);
    uint64_t elem_size = ndl_vector_elem_size(vec);
    uint64_t index = ((uint64_t) ((uint8_t *) elem - (uint8_t *) base)) / elem_size;

    if (index == 0)
        return NULL;

    uint64_t pindex = (index - 1) >> 1;

    return (void *) (((uint8_t *) base) + elem_size * pindex);
}


static inline void *ndl_heap_lchild(ndl_vector *vec, void *elem) {

    void *base = ndl_vector_get(vec, 0);
    uint64_t elem_size = ndl_vector_elem_size(vec);
    uint64_t index = ((uint64_t) ((uint8_t *) elem - (uint8_t *) base)) / elem_size;

    uint64_t cindex = (index << 1) + 1;

    if (cindex >= ndl_vector_size(vec))
        return NULL;

    return (void *) (((uint8_t *) base) + elem_size * cindex);
}

static inline void *ndl_heap_rchild(ndl_vector *vec, void *elem) {

    void *base = ndl_vector_get(vec, 0);
    uint64_t elem_size = ndl_vector_elem_size(vec);
    uint64_t index = ((uint64_t) ((uint8_t *) elem - (uint8_t *) base)) / elem_size;

    uint64_t cindex = (index << 1) + 2;

    if (cindex >= ndl_vector_size(vec))
        return NULL;

    return (void *) (((uint8_t *) base) + elem_size * cindex);
}

static inline void *ndl_heap_bubble(ndl_heap *heap, void *node) {

    ndl_vector *vec = (ndl_vector *) heap->vector;

    void *parent = ndl_heap_parent(vec, node);
    while (parent != NULL) {

        if (heap->compare(node, parent) <= 0)
            return node;

        heap->swap(node, parent);

        node = parent;
        parent = ndl_heap_parent(vec, node);
    }

    return node;
}

static inline void *ndl_heap_sink(ndl_heap *heap, void *node) {

    ndl_vector *vec = (ndl_vector *) heap->vector;

    while (1) {

        void *best;

        void *lchild = ndl_heap_lchild(vec, node);
        if (lchild == NULL)
            return node;

        void *rchild = ndl_heap_rchild(vec, node);

        if (rchild == NULL)
            best = lchild;
        else {
            best = (heap->compare(lchild, rchild) < 0)? rchild : lchild;
        }

        if (heap->compare(node, best) < 0) {

            heap->swap(node, best);

            node = best;

            continue;
        }

        return node;
    }
}

static inline void ndl_heap_delete(ndl_heap *heap, void *node) {

    ndl_vector *vec = (ndl_vector *) heap->vector;

    uint64_t size = ndl_vector_size(vec);
    void *last = ndl_vector_get(vec, size - 1);

    memmove(node, last, (size_t) ndl_vector_elem_size(vec));

    ndl_vector_pop(vec);

    node = ndl_heap_bubble(heap, node);
    ndl_heap_sink(heap, node);
}

int ndl_heap_pop(ndl_heap *heap) {

    ndl_vector *vec = (ndl_vector *) heap->vector;
    if (ndl_vector_size(vec) == 0)
        return -1;

    ndl_heap_delete(heap, ndl_vector_get(vec, 0));

    return 0;
}

void *ndl_heap_peek(ndl_heap *heap) {

    ndl_vector *vec = (ndl_vector *) heap->vector;
    if (ndl_vector_size(vec) == 0)
        return NULL;

    return ndl_vector_get(vec, 0);
}

void *ndl_heap_head(ndl_heap *heap) {

    ndl_vector *vec = (ndl_vector *) heap->vector;
    if (ndl_vector_size(vec) == 0)
        return NULL;

    return ndl_vector_get(vec, 0);
}
/* TODO: Move iteration logic to vector? */
void *ndl_heap_next(ndl_heap *heap, void *prev) {

    if (prev == NULL)
        return NULL;

    ndl_vector *vec = (ndl_vector *) heap->vector;
    uint64_t size = ndl_vector_size(vec);
    if (size == 0)
        return NULL;

    uint8_t *max = ndl_vector_get(vec, ndl_vector_size(vec) - 2);

    if (prev <= (void *) max)
        return (void *) (((uint8_t *) prev) + ndl_vector_elem_size(vec));

    return NULL;
}

void *ndl_heap_readj(ndl_heap *heap, void *data) {

    data = ndl_heap_bubble(heap, data);
    return ndl_heap_sink(heap, data);
}

void *ndl_heap_put(ndl_heap *heap, void *data) {

    ndl_vector *vec = (ndl_vector *) heap->vector;
    void *res = ndl_vector_push(vec, data);
    if (res == NULL)
        return NULL;

    res = ndl_heap_bubble(heap, res);

    return res;
}

int ndl_heap_del(ndl_heap *heap, void *data) {

    ndl_heap_delete(heap, data);

    ndl_vector_pop((ndl_vector *) heap->vector);

    return 0;
}

uint64_t ndl_heap_data_size(ndl_heap *heap) {

    return ndl_vector_elem_size((ndl_vector *) heap->vector);
}

ndl_heap_cmp_func ndl_heap_compare(ndl_heap *heap) {

    return heap->compare;
}

ndl_heap_swap_func ndl_heap_swap(ndl_heap *heap) {

    return heap->swap;
}

uint64_t ndl_heap_size(ndl_heap *heap) {

    return ndl_vector_size((ndl_vector *) heap->vector);
}

uint64_t ndl_heap_cap(ndl_heap *heap) {

    return ndl_vector_cap((ndl_vector *) heap->vector);
}

void ndl_heap_print(ndl_heap *heap) {

    printf("Printing heap.\n");
    printf("Compare: %p.\n", (void *) &heap->compare);
    ndl_vector_print((ndl_vector *) heap->vector);
}
