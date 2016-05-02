#ifndef NODEL_EXCALL_H
#define NODEL_EXCALL_H

#include <stdint.h>

#include "node.h"
#include "rehashtable.h"

/* Excall table management for evaluation.
 *
 * Excalls can modify the graph, the process, anything.
 * Process may not exist after excall.
 * Excalls are given the invoking instruction and the
 * process local frame, *after* advancement.
 */

typedef ndl_rhashtable ndl_excall;

typedef void (*ndl_excall_func)(ndl_ref *inst, ndl_ref *local);

/* Create and destroy excall tables.
 *
 * init() creates an excall table and initializes it.
 *     Returns NULL on error.
 * kill() frees an excall table and its memory.
 *
 * minit() creates an excall table in the given memory region.
 *     Returns NULL on error.
 * mkill() frees an excall table, but not its memory.
 * msize() returns the size needed for a given excall table.
 */
ndl_excall *ndl_excall_init(void);
void        ndl_excall_kill(ndl_excall *table);

ndl_excall *ndl_excall_minit(void *region);
void        ndl_excall_mkill(ndl_excall *table);
uint64_t    ndl_excall_msize(void);

/* Excall registration.
 *
 * put() adds an excall to the table.
 *     Returns zero on success, nonzero on error.
 * del() removes an excall from the table.
 *     Returns zero on success, nonzero on error.
 * get() gets an excall from the table.
 *     Returns pointer on success, NULL on error.
 */
int ndl_excall_put(ndl_excall *table, ndl_sym name, ndl_excall_func func);
int ndl_excall_del(ndl_excall *table, ndl_sym name);
ndl_excall_func ndl_excall_get(ndl_excall *table, ndl_sym name);

/* Iterate over current excalls.
 * Iterators are invalidated on mutating operation.
 *
 * head() gets the first excall iterator.
 *     Returns NULL on end-of-list or error.
 * next() gets the next excall iterator.
 *     Returns NULL on end-of-list or error.
 *
 * key() returns the key for the current excall.
 *     Returns NDL_NULL_SYM on error.
 * func() returns the function for the current excall.
 *     Returns NULL on error.
 */
void *ndl_excall_head(ndl_excall *table);
void *ndl_excall_next(ndl_excall *table, void *prev);

ndl_sym         ndl_excall_key(ndl_excall *table, void *curr);
ndl_excall_func ndl_excall_val(ndl_excall *table, void *curr);

#endif /* NODEL_EXCALL_H */
