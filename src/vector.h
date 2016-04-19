#ifndef NODEL_VECTOR_H
#define NODEL_VECTOR_H

#include <stdint.h>

/* Malloc based resizable vector.
 * Amortized O(1) push/pop back, O(n) any other insert/delete
 * operation. Guaranteed to be contiguous, but
 * pointer may go bad between mutating calls.
 */

typedef struct ndl_vector_s {

    uint64_t elem_size;
    uint64_t elem_count, elem_cap;

    uint8_t *data;

} ndl_vector;

/* Initialize and free vectors and their elements.
 *
 * init() allocates and initializes a vector with the given parameters.
 * kill() frees a vector.
 *
 * minit() initializes a vector in the given region of memory.
 * mkill() kills a vector without freeing its root structure.
 * msize() gets the size of a vector's root structure.
 */
ndl_vector *ndl_vector_init(uint64_t elem_size);
void        ndl_vector_kill(ndl_vector *vector);

ndl_vector *ndl_vector_minit(void *region, uint64_t elem_size);
void        ndl_vector_mkill(ndl_vector *vector);
uint64_t    ndl_vector_msize(uint64_t elem_size);

/* Vector element manipulation.
 *
 * get() gets the element with the given index, or NULL.
 *
 * push() pushes an element to the back of the vector.
 * pop() deletes the element at the back of the vector.
 *     Returns 0 on success, nonzero on error.
 *
 * delete() deletes the indexed element.
 *     Returns 0 on success, nonzero on error.
 * insert() inserts the given element at the given index.
 *
 * delete_range() deletes n elements from the given index onwards.
 * insert_range() inserts n elements into the given index.
 */
void *ndl_vector_get(ndl_vector *vector, uint64_t index);

void *ndl_vector_push(ndl_vector *vector, void *elem);
int   ndl_vector_pop (ndl_vector *vector);

int   ndl_vector_delete(ndl_vector *vector, uint64_t index);
void *ndl_vector_insert(ndl_vector *vector, uint64_t index, void *elem);

int   ndl_vector_delete_range(ndl_vector *vector, uint64_t index, uint64_t len);
void *ndl_vector_insert_range(ndl_vector *vector, uint64_t index, uint64_t len, void *elems);

/* Vector metadata.
 *
 * size() gets the number of elements.
 * cap() gets the capacity in the current backend.
 *
 * elem_size() gets the size of each element.
 */
uint64_t ndl_vector_size(ndl_vector *vector);
uint64_t ndl_vector_cap (ndl_vector *vector);

uint64_t ndl_vector_elem_size(ndl_vector *vector);

/* Print the vector's metadata. */
void ndl_vector_print(ndl_vector *vector);

#endif /* NODEL_VECTOR_H */
