#ifndef NODEL_RUNTIME_H
#define NODEL_RUNTIME_H

#include "graph.h"

/* Runtimes contain the data necessary to run a number of nodel
 * processes in a graph. They assign PIDs to each process, store
 * related data, and provide functions for manipulating these processes.
 *
 * There are two ways to use a runtime.
 * The first is to manually step some or all of the processes at your leisure.
 * The second is to assign each process a frequency to run at, and repeatedly
 * call the run_* functions to run sleeping procesess, waiting processes,
 * processes set to run at specific frequencies, and suspended processes at
 * appropriate times.
 *
 * Suspended processes are not waiting or sleeping on any event. They are not running.
 * Waiting processes are waiting for a node to be modified.
 * Sleeping processes are waiting for some timeout to occur.
 * Running processes are running at some set frequency.
 * (If frequency is >10MHz, runs as fast as possible.)
 * Frequencies are specified in deciHertz for the moment.
 * Waiting/sleeping/suspended processes still have frequencies, and will
 * use them upon revival.
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
 * proc_delsleep() wakes up a sleeping process.
 *
 * proc_setwait() sleeps a process for the given interval (usec).
 * proc_delwait() wakes up a waiting process.
 */
int ndl_runtime_proc_suspend(ndl_runtime *runtime, int64_t pid);
int ndl_runtime_proc_resume (ndl_runtime *runtime, int64_t pid);

int     ndl_runtime_proc_setfreq(ndl_runtime *runtime, int64_t pid, int64_t freq);
int64_t ndl_runtime_proc_getfreq(ndl_runtime *runtime, int64_t pid);

int ndl_runtime_proc_setsleep(ndl_runtime *runtime, int64_t pid, uint64_t interval);
int ndl_runtime_proc_delsleep(ndl_runtime *runtime, int64_t pid);

int ndl_runtime_proc_setwait(ndl_runtime *runtime, int64_t pid, ndl_ref node);
int ndl_runtime_proc_delwait(ndl_runtime *runtime, int64_t pid);

/* Run the runtime using the scheduler system and events.
 * For an explanation of the scheduler and event system, see above.
 *
 * run_step() runs each ready process for at most one cycle.
 * run_finish() runs each ready process until no processes are ready, or
 *     the provided timeout is reached. Timeout in usec, ignores if 0.
 *     If the timeout is reached, returns -1. Else, 0.
 *
 * run_sleep() sleeps the thread until the next event is ready.
 *     Returns number of usec slept.
 * run_timeto() returns the time until the next event in usec.
 *     Returns zero if next event has already passed.
 *
 * run_for() calls run_step() and run_sleep() until the given timeout (usec)
 *     is reached, or there are no processes left. If timeout is negative,
 *     does not use timeout.
 */
int ndl_runtime_run_step  (ndl_runtime *runtime);
int ndl_runtime_run_finish(ndl_runtime *runtime, int64_t timeout);

uint64_t ndl_runtime_run_sleep (ndl_runtime *runtime);
uint64_t ndl_runtime_run_timeto(ndl_runtime *runtime);

int ndl_runtime_run_for(ndl_runtime *runtime, int64_t timeout);

/* Run processes manually.
 * Does not interact with the scheduling system.
 * Use cautiously.
 *
 * step_proc() runs the process with the given PID the given number of steps.
 * step() steps all processes the given number of steps.
 */
void ndl_runtime_step_proc(ndl_runtime *runtime, int64_t pid, uint64_t steps);
void ndl_runtime_step     (ndl_runtime *runtime,              uint64_t steps);

/* Enumerate processes.
 * These pointers are __INVALID__ after any other runtime operations.
 * Do not intermix.
 *
 * prochead() gets a pointer to the first PID in the list.
 *     Returns NULL at end-of-list.
 * procnext() gets a pointer to the next PID in the list.
 *     Returns NULL at end-of-list.
 */
int64_t *ndl_runtime_prochead(ndl_runtime *runtime);
int64_t *ndl_runtime_procnext(ndl_runtime *runtime, int64_t *last);

/* Runtime metadata.
 *
 * graph() gets the runtime's graph.
 * graphown() returns 0 if the graph was lent, 1 if malloc'd().
 * proc_count() gets the number of processes in the runtime.
 */
ndl_graph *ndl_runtime_graph     (ndl_runtime *runtime);
int        ndl_runtime_graphown  (ndl_runtime *runtime);
uint64_t   ndl_runtime_proc_count(ndl_runtime *runtime);


/* Print the entire runtime to console. */
void ndl_runtime_print(ndl_runtime *runtime);

#endif /* NODEL_RUNTIME_H */
