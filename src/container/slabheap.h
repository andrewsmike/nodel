#ifndef NODEL_SLABHEAP_H
#define NODEL_SLABHEAP_H

#include <stdint.h>

#include "slab.h"

/* Generic max binary heap implemented over slab allocator.
 * O(log(n)) everything ignoring allocations, amortized O(log(n))
 * including (rare, and small) allocations/moves.
 * Slab root struct built into slabheap structure.
 */

typedef struct ndl_slabheap_node_s {

    ndl_slab_index sid;

    struct ndl_slabheap_node_s *parent;
    struct ndl_slabheap_node_s *left, *right;

    uint8_t data[];

} ndl_slabheap_node;

typedef int (*ndl_slabheap_cmp_func)(void *a, void *b);

typedef struct ndl_slabheap_s {

    uint64_t data_size;
    ndl_slabheap_cmp_func compare;

    ndl_slabheap_node *root, *foot;
    uint8_t slab[];

} ndl_slabheap;


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
ndl_slabheap *ndl_slabheap_init(uint64_t data_size, ndl_slabheap_cmp_func compare,
                                uint64_t slab_block_size);
void          ndl_slabheap_kill(ndl_slabheap *heap);

ndl_slabheap *ndl_slabheap_minit(void *region, uint64_t data_size,
                                 ndl_slabheap_cmp_func compare, uint64_t slab_block_size);
void     ndl_slabheap_mkill(ndl_slabheap *heap);
uint64_t ndl_slabheap_msize(uint64_t data_size, ndl_slabheap_cmp_func compare,
                            uint64_t slab_block_size);

/* Get head, peek head, put, and delete items from the heap.
 * Pointers are stable; you don't have to re-get()
 * whenever you put or delete.
 *
 * head() returns the top data and frees it. The pointer will be valid
 *    until the next put operation.
 * peek() returns a pointer to the top data without deleting it.
 *
 * readj() Readjusts a modified node, moving it up or down in the
 *     heap as needed.
 *
 * put() adds a node to the heap.
 * del() deletes a node from the heap.
 *     Returns 0 on success, -1 on error.
 */
void *ndl_slabheap_head(ndl_slabheap *heap);
void *ndl_slabheap_peek(ndl_slabheap *heap);

void  ndl_slabheap_readj(ndl_slabheap *heap, void *data);

void *ndl_slabheap_put(ndl_slabheap *heap, void *data);
int   ndl_slabheap_del(ndl_slabheap *heap, void *data);

/* Heap element iteration.
 * Items unordered.
 *
 * node_head() gets the first node in the slab.
 *     Returns NULL at end-of-list.
 * node_next() gets the next node in the slab.
 *     Returns NULL at end-of-list.
 */
void *ndl_slabheap_node_head(ndl_slabheap *heap);
void *ndl_slabheap_node_next(ndl_slabheap *heap, void *prev);

/* Slabheap metadata.
 *
 * data_size() gets the sizeof(data) for the slabheap.
 * cmp_func() returns the comparison function for the slabheap.
 *
 * size() gets the number of nodes in the slabheap.
 * cap() gets the *current* capacity, without growing the slab.
 */
uint64_t              ndl_slabheap_data_size(ndl_slabheap *heap);
ndl_slabheap_cmp_func ndl_slabheap_compare  (ndl_slabheap *heap);

uint64_t ndl_slabheap_size(ndl_slabheap *heap);
uint64_t ndl_slabheap_cap (ndl_slabheap *heap);

/* Print the entirety of the slabheap. */
void ndl_slabheap_print(ndl_slabheap *heap);

#endif /* NODEL_SLABHEAP_H */
