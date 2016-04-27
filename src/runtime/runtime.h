#ifndef NODEL_RUNTIME_H
#define NODEL_RUNTIME_H

#include "graph.h"

/* Runtimes represent some number of processes acting on
 * a graph. It indexes processes by a numeric identifier.
 *
 * Processes have two primary forms of state:
 * How the process is being executed (external), and
 * whether the process is running, waiting, sleeping, or dead (internal).
 *
 * Processes move between the internal states during the normal course
 * of their execution. External actors may deactivate or reactivate processes
 * as needed.
 *
 * See proc.h for more details.
 *
 */

typedef struct ndl_runtime_s ndl_runtime;

typedef enum ndl_proc_state_e {

    ESTATE_NONE, /* Error. */

    ESTATE_RUNNING,
    ESTATE_SUSPENDED,
    ESTATE_SLEEPING,
    ESTATE_WAITING,

    ESTATE_SIZE

} ndl_proc_state;


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
 * proc_init() makes a new process from the given local block.
 *     Returns the PID on success, -1 on error.
 *     Process starts off as suspended.
 * proc_kill() deletes a process with the given PID.
 *     Returns 0 on success, nonzero on error.
 */
int64_t ndl_runtime_proc_init(ndl_runtime *runtime, ndl_ref local);
int     ndl_runtime_proc_kill(ndl_runtime *runtime, int64_t pid);

/* Manipulate running state of processes.
 *
 * Processes can be suspended, can be waiting on a node,
 * can be sleeping (waiting on the clock), or can be running
 * at a set frequency.
 *
 * proc_getstate() returns the state of a process, or ESTATE_NONE on error.
 *
 * proc_suspend() suspends the PID. Returns 0 on success, -1 on error.
 * proc_resume() wakes up a suspended thread.
 *
 * proc_setfreq() sets the frequency for a PID. Returns 0 on success, -1 on error.
 *     Does not wake up a sleeping/waiting/suspended process.
 *     They will have this freq when they resume.
 *     __Frequency in deci-Hertz.__
 * proc_getfreq() returns the frequency for a PID. Returns -1 on error.
 *
 * proc_setsleep() sleeps a process until the given time.
 *     Use proc_suspend() or proc_resume to unsleep and / or resume process.
 *
 * proc_setwait() waits a process on a given node.
 *     Use proc_suspend() or proc_resume to unwait and / or resume process.
 */
ndl_proc_state ndl_runtime_proc_getstate(ndl_runtime *runtime, int64_t pid);

int ndl_runtime_proc_suspend(ndl_runtime *runtime, int64_t pid);
int ndl_runtime_proc_resume (ndl_runtime *runtime, int64_t pid);

int     ndl_runtime_proc_setfreq(ndl_runtime *runtime, int64_t pid, uint64_t freq);
int64_t ndl_runtime_proc_getfreq(ndl_runtime *runtime, int64_t pid);

int ndl_runtime_proc_setsleep(ndl_runtime *runtime, int64_t pid, uint64_t interval);

int ndl_runtime_proc_setwait(ndl_runtime *runtime, int64_t pid, ndl_ref node);

/* Run the runtime using the scheduler system and events.
 * For an explanation of the scheduler and event system, see above.
 *
 * run_ready() runs each ready process until no processes are ready, or
 *     the provided timeout is reached. Timeout in usec, ignores if negative.
 *     Timeout resolution is pretty low.
 *     Returns zero on success, -1 on error, and 1 on timeout. 
 *
 * run_sleep() sleeps the thread until the next event is ready.
 *     Returns number of usec slept, or negative on error.
 *     If timeout is positive, returns at min(timeout, timeto).
 * run_timeto() returns the time until the next event in usec.
 *     Returns zero if next event has already passed. Returns -1 on error.
 *
 * run_for() calls run_step() and run_sleep() until the given timeout (usec)
 *     is reached, or there are events left. If timeout is negative,
 *     does not use timeout.
 */
int ndl_runtime_run_ready(ndl_runtime *runtime, int64_t timeout);

int64_t ndl_runtime_run_sleep (ndl_runtime *runtime, int64_t timeout);
int64_t ndl_runtime_run_timeto(ndl_runtime *runtime);

int ndl_runtime_run_for(ndl_runtime *runtime, int64_t timeout);

/* Enumerate processes.
 * These pointers are __INVALID__ after any other runtime operations.
 * Do not intermix.
 *
 * proc_head() gets a pointer to the first PID in the list.
 *     Returns NULL at end-of-list.
 * proc_next() gets a pointer to the next PID in the list.
 *     Returns NULL at end-of-list.
 */
int64_t *ndl_runtime_proc_head(ndl_runtime *runtime);
int64_t *ndl_runtime_proc_next(ndl_runtime *runtime, int64_t *last);

/* Runtime metadata.
 *
 * graph() gets the runtime's graph.
 * graphown() returns 0 if the graph was lent, 1 if malloc'd().
 * dead() returns 0 if there are events, 1 if there are no waiting events.
 * proc_count() gets the number of processes in the runtime.
 */
ndl_graph *ndl_runtime_graph      (ndl_runtime *runtime);
int        ndl_runtime_graphown   (ndl_runtime *runtime);
int        ndl_runtime_dead       (ndl_runtime *runtime);
uint64_t   ndl_runtime_proc_count (ndl_runtime *runtime);

/* Print the entire runtime to console. */
void ndl_runtime_print(ndl_runtime *runtime);

#endif /* NODEL_RUNTIME_H */
