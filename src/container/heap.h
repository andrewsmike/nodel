#ifndef NODEL_HEAP_H
#define NODEL_HEAP_H

#include <stdint.h>

/* Generic max binary heap implemented over a vector.
 * Typically O(log(n)).
 * Compare returns -1:less than, 0:equals, 1:greater than.
 */

typedef int (*ndl_heap_cmp_func)(void *a, void *b);

typedef struct ndl_heap_s {

    ndl_heap_cmp_func compare;

    uint8_t vector[];

} ndl_heap;


/* Create and destroy heaps.
 * Heaps must be killed before freed.
 *
 * init() allocates and initialize a heap.
 * kill() cleans up and frees a heap.
 *
 * minit() initializes a heap in the given memory region.
 * mkill() cleans up a heap without freeing it.
 * msize() gets the size needed to store a heap.
 */
ndl_heap *ndl_heap_init(uint64_t data_size, ndl_heap_cmp_func compare);

void      ndl_heap_kill(ndl_heap *heap);

ndl_heap *ndl_heap_minit(void *region, uint64_t data_size, ndl_heap_cmp_func compare);
void      ndl_heap_mkill(ndl_heap *heap);
uint64_t  ndl_heap_msize(uint64_t data_size, ndl_heap_cmp_func compare);

/* Get head, peek head, put, and delete items from the heap.
 * Pointers are invalidated on modifying operations.
 *
 * pop() deletes the top element of the heap.
 * peek() returns a pointer to the top data without deleting it.
 *
 * readj() Readjusts a modified node, moving it up or down in the
 *     heap as needed. Returns new placement.
 *
 * put() adds a node to the heap.
 * del() deletes a node from the heap.
 *     Returns 0 on success, -1 on error.
 */
int   ndl_heap_pop (ndl_heap *heap);
void *ndl_heap_peek(ndl_heap *heap);

void *ndl_heap_readj(ndl_heap *heap, void *data);

void *ndl_heap_put(ndl_heap *heap, void *data);
int   ndl_heap_del(ndl_heap *heap, void *data);

/* Heap element iteration.
 * Items unordered.
 * Iterator invalidated on mutating operation.
 *
 * node_head() gets the first node in the heap.
 *     Returns NULL at end-of-list.
 * node_next() gets the next node in the heap.
 *     Returns NULL at end-of-list.
 */
void *ndl_heap_head(ndl_heap *heap);
void *ndl_heap_next(ndl_heap *heap, void *prev);

/* Heap metadata.
 *
 * data_size() gets the sizeof(data) for the heap.
 * cmp_func() returns the comparison function for the heap.
 *
 * size() gets the number of nodes in the heap.
 * cap() gets the *current* capacity, without growing the vector.
 */
uint64_t          ndl_heap_data_size(ndl_heap *heap);
ndl_heap_cmp_func ndl_heap_compare  (ndl_heap *heap);

uint64_t ndl_heap_size(ndl_heap *heap);
uint64_t ndl_heap_cap (ndl_heap *heap);

/* Print the entirety of the heap. */
void ndl_heap_print(ndl_heap *heap);

#endif /* NODEL_HEAP_H */
