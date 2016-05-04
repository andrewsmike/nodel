#ifndef NODEL_RUNTIME_H
#define NODEL_RUNTIME_H

typedef struct ndl_runtime_s ndl_runtime;

#include "rehashtable.h"
#include "ndltime.h"
#include "heap.h"

#include "graph.h"
#include "proc.h"
#include "excall.h"

/* Runtimes are a number of processes working on a graph.
 * Runtime holds the logic to run processes, suspend them,
 * and supports their various features (excalls, sleep, etc.)
 *
 * See proc.h for more details on process state.
 */

/* Structure stored in heap for
 * getting the first clock event.
 */
typedef struct ndl_runtime_clockevent_s {

    ndl_time when;
    ndl_pid head;
    ndl_runtime *runtime;

} ndl_runtime_clockevent;

/* Holds all information necessary for a runtime to run.
 * Includes the following:
 * - Graph, and whether or not to graph_kill() on runtime_kill().
 * - PID -> ndl_proc hashtable. (Not a pointer, actual procs.)
 * - clock event heap
 * - wait event table (ref -> pid (event list head))
 */
struct ndl_runtime_s {

    /* Graph to use. */
    int free_graph;
    ndl_graph *graph;

    /* PID table. */
    ndl_pid next_pid;
    ndl_rhashtable *procs;

    /* Event hooks. */

    /* Events that happen after a timeout.
     * Gets the most immediate event list head
     */
    ndl_heap *clockevents;

    /* Node modify table for wait().
     * Maps from node ID to event list head.
     */
    ndl_rhashtable *waitevents;
};

/* Create and destroy a runtime.
 * Runtimes encapsulate a node graph and a number of processes.
 *
 * init() creates a new runtime. If graph is NULL, creates own graph.
 * kill() frees all runtime resources and the given runtime.
 *
 * minit() creates a new runtime in the given memory region.
 *     If graph is not NULL, uses that graph instead.
 * mkill() frees all runtime resources, but not the runtime's region.
 *     If the graph was provided at creation, does not delete. 
 * msize() gets the sizeof(ndl_runtime) given the arguments.
 */
ndl_runtime *ndl_runtime_init(ndl_graph *graph);
void         ndl_runtime_kill(ndl_runtime *runtime);

ndl_runtime *ndl_runtime_minit(void *region, ndl_graph *graph);
void         ndl_runtime_mkill(ndl_runtime *runtime);
uint64_t     ndl_runtime_msize(ndl_graph *graph);

/* Create and destroy processes.
 *
 * proc() gets a process give its PID.
 *     Returns NULL on error.
 *
 * proc_init() creates a new process with the given local block and period.
 *     Returns the new process with PID set, NULL on error.
 *     Process starts in inactive:running state.
 * proc_kill() deletes a process with the given PID.
 *     Returns 0 on success, nonzero on error.
 *     This collects dead processes. This also kills living ones.
 */
ndl_proc *ndl_runtime_proc(ndl_runtime *runtime, ndl_pid pid);

ndl_proc *ndl_runtime_proc_init(ndl_runtime *runtime,
                                ndl_ref local, ndl_time period);
int       ndl_runtime_proc_kill(ndl_runtime *runtime, ndl_proc *proc);

/* Process iterator.
 *
 * proc_head() gets the first process iterator.
 *     Returns NULL at end-of-list or error.
 * proc_next() gets the next process iterator.
 *     Returns NULL at end-of-list or error.
 *
 * proc_pid() gets the PID of a process iterator.
 *     Returns NDL_NULL_PID on error.
 * proc_proc() gets the proc of a process iterator.
 *     Returns NULL on error.
 */
void *ndl_runtime_proc_head(ndl_runtime *runtime);
void *ndl_runtime_proc_next(ndl_runtime *runtime, void *prev);

ndl_pid   ndl_runtime_proc_pid (ndl_runtime *runtime, void *curr); 
ndl_proc *ndl_runtime_proc_proc(ndl_runtime *runtime, void *curr);

/* Runtime metadata.
 *
 * graph() gets a runtime's graph.
 * graph_free() returns whether the graph will (1) or will not (0)
 *     be deleted when the runtime is kill()d.
 *
 * proc_count() gets the number of processes in the runtime.
 * proc_alive() gets the number of active processes in the runtime.
 * proc_alive() gets whether there are processes running or sleeping.
 */
ndl_graph *ndl_runtime_graph     (ndl_runtime *runtime);
int        ndl_runtime_graph_free(ndl_runtime *runtime);

uint64_t ndl_runtime_proc_count(ndl_runtime *runtime);
uint64_t ndl_runtime_proc_living(ndl_runtime *runtime);
int      ndl_runtime_proc_alive(ndl_runtime *runtime);

/* Run the runtime using the clock event system.
 * Timeouts are absolute times (start + duration), rather than relative.
 *
 * run_ready() runs each ready process until no processes are ready, or
 *     the provided timeout is reached. Timeout ignored if zero.
 *     Timeout resolution is pretty low.
 *     Timeout is relative (ends before (now + timeout.))
 *     Returns zero on success, and -1 on error.
 *
 * run_sleep() sleeps the thread until the next event is ready.
 *     Returns the time slept. If error, does not sleep.
 *     If timeout is NDL_TIME_ZERO, does not timeout.
 *     Timeout is relative (ends before (now + timeout.))
 * run_timeto() returns the time until the next event.
 *     Returns NDL_TIME_ZERO if we're running late,
 *     there are no processes left, or on error.
 *
 * run_for() calls run_step() and run_sleep() repeatedly.
 *     If timeout is not NDL_TIME_ZERO, attempts to exit before timeout.
 *     Timeout is relative (ends before (now + timeout.))
 *     Returns zero if timeout is reached or runtime is inactive.
 *     Returns nonzero on error.
 */
int ndl_runtime_run_ready(ndl_runtime *runtime, ndl_time timeout);

ndl_time ndl_runtime_run_sleep (ndl_runtime *runtime, ndl_time timeout);
ndl_time ndl_runtime_run_timeto(ndl_runtime *runtime);

int ndl_runtime_run_for(ndl_runtime *runtime, ndl_time timeout);

/* Print the entire runtime to console. */
void ndl_runtime_print(ndl_runtime *runtime);

#endif /* NODEL_RUNTIME_H */
