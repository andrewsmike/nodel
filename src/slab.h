#ifndef NODEL_SLAB_H
#define NODEL_SLAB_H

#include <stdint.h>
#include <limits.h>

/* A slab allocates large blocks of a given size at once,
 * rather than relying on malloc to pack them nicely.
 *
 * More importantly for this system, it provides a way of
 * indexing all allocated objects with a single integer.
 * This slab system cannot be shrunk once grown, as IDs
 * are based on position, but is efficient for the given workload.
 *
 * This system allows for the O(1) indexing, allocating, and freeing
 * of resources.
 */

/* Each allocated item has a slot for storing a next reference.
 * The slab system keeps all free blocks on a linked list for quick allocation.
 * The last element has a reference to blocksize*blockcount.
 * The reference is NDL_NULL_INDEX when unallocated.
 * The data is arbitrarily sized. Use ((struct my_struct *) item.data).
 */
typedef struct ndl_slab_item_s {

    uint64_t next_free;
    uint8_t data[];

} ndl_slab_item;

/* Blocks are just a vector of items.
 * The number and size of items both depend on the slab's
 * initializing parameters.
 */
typedef struct ndl_slab_block_s ndl_slab_block;

/* Slabs store the element size, the prefered number per block,
 * the number of blocks, the capacity of the block pointer vector,
 * the number of unallocated elements, and the first reference on
 * the free list for allocation / freeing. Using the same list for
 * iteration would require a doubly linked list, so we opt for scanning.
 * The blocks pointer can be NULL if block_count is 0.
 */
typedef struct ndl_slab_s {

    uint64_t elem_size, block_size;
    uint64_t block_count, block_cap;
    uint64_t elem_count;

    uint64_t free_head;

    ndl_slab_block **blocks;

} ndl_slab;

/* Use unsigned 64 bit integers for indices. */
typedef uint64_t ndl_slab_index;
#define NDL_NULL_INDEX ((ndl_slab_index) ULLONG_MAX)


/* Initilaize and free slabs and their elements.
 *
 * init() creates a slab with the given parameters.
 *     If NDL_NULL_INDEX is given for block_size, it picks block_size
 *     such that block_size * elem_size ~= some nice constant.
 * kill() frees the entire slab structure.
 */
ndl_slab *ndl_slab_init(uint64_t elem_size, uint64_t block_size);
void      ndl_slab_kill(ndl_slab *slab);

/* Allocation and indexing of slab elements.
 *
 * alloc() allocates a new element. Returns NDL_NULL_INDEX on error.
 * free()  frees an element. Does not fail.
 * index() Returns a pointer to the element with the given index.
 *     Returns NULL on error.
 */
ndl_slab_index ndl_slab_alloc(ndl_slab *slab);
void           ndl_slab_free (ndl_slab *slab, ndl_slab_index index);
void          *ndl_slab_get  (ndl_slab *slab, ndl_slab_index index);

/* Slab element iteration.
 *
 * head() returns the first allocated element in the slab.
 * next() gets the next allocated element from the slab.
 *     Returns NDL_NULL_INDEX on error or at end of list.
 */
ndl_slab_index ndl_slab_head(ndl_slab *slab);
ndl_slab_index ndl_slab_next(ndl_slab *slab, ndl_slab_index last);

/* Slab metadata.
 *
 * size()       returns the total current size (in elems) of the slab.
 * elem_count() returns the number of active elements in the current slab.
 * free_count() returns the number of free elemnts in the current slab.
 */
uint64_t ndl_slab_size      (ndl_slab *slab);
uint64_t ndl_slab_elem_count(ndl_slab *slab);
uint64_t ndl_slab_free_count(ndl_slab *slab);

#endif /* NODEL_SLAB_H */
