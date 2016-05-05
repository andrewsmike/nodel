#ifndef NODEL_PROC_H
#define NODEL_PROC_H

#include "node.h"
#include "ndltime.h"

/* Processes describe a process and manage its event logic.
 * There are four primary states. In addition to these states,
 * a process is also inactive or active, deciding whether it is
 * 'hooked' into the rest of the event system surrounding the runtime.
 *
 * The primary internal state is described below.
 * - Sleeping: Waiting for a timeout after a sleep() call.
 * - Waiting: Waiting for a node to be modified after a wait() call.
 * - Running: Currently running.
 * - Dead: Dead, to be collected on.
 *
 * Processes themselves are passive; they wait for events.
 * In each state, the process is waiting for an external actor to trigger an event.
 * - Sleeping: Registered with clock event heap.
 * - Waiting: Registered with node modification event table.
 * - Running: Registered with clock event heap.
 * - Dead: Nothing.
 *
 *
 * +---------+ <--Event(wait(node))-- +---------+ <--Event(timeout)----------- +----------+
 * | Waiting |                        | Running |                              | Sleeping |
 * +---------+ ---Event(modified)---> +---------+ ---Event(sleep(interval))--> +----------+
 *           \                             |                                   /
 *            -------                      | Event(exit())              -------
 *                   \ Event(killed)       | Event(error)        Event(killed)
 *                     -------             | Event(killed)      -------
 *                            \            V                   /
 *                             ------> +------+ <--------------
 *                                     | Dead |
 *                                     +------+
 */

typedef enum ndl_proc_state_e {

    ESTATE_NULL, /* Process does not exist. */

    ESTATE_RUNNING,

    ESTATE_SLEEPING,
    ESTATE_WAITING,

    ESTATE_DEAD,

    ESTATE_SIZE
} ndl_proc_state;

typedef enum ndl_proc_reason_e {

    ECAUSE_NULL,     /* Process does not exist. */

    ECAUSE_NONE,     /* Process is not dead. */

    ECAUSE_EXIT,     /* Process called exit(). */
    ECAUSE_KILLED,   /* Process was killed externally. */
    ECAUSE_BAD_INST, /* Process encountered bad instruction. */
    ECAUSE_BAD_DATA, /* Process encountered bad data. */
    ECAUSE_INTERNAL, /* System encountered internal error. */

    ECAUSE_SIZE

} ndl_proc_reason;

typedef int64_t ndl_pid;
#define NDL_NULL_PID ((ndl_pid) -1)

typedef struct ndl_proc_s ndl_proc;
#include "runtime.h"

struct ndl_proc_s {

    /* General state */
    ndl_runtime *runtime;
    ndl_pid pid;

    int active;
    ndl_proc_state state;

    /* Event list. */
    ndl_pid event_prev, event_next;
    void *head;

    /* Local frame / process data. */
    ndl_ref local;   /* Local frame. */
    ndl_time period; /* Period between cycles, NDL_TIME_ZERO for unlimited. */

    /* State specific data. */
    union {

        /* Sleeping. */
        ndl_time duration; /* If active: when to wake up, else time left. */

        /* Waiting. */
        ndl_ref waiting; /* Node to wait on. */

        /* Dead. */
        ndl_proc_reason cause_of_death; /* What killed us? */
    };

};

/* Create and destroy processes.
 *
 * minit() creates a process in the given region of memory.
 *     Does not fail.
 *     Does not register process with runtime.
 * mkill() destroys a process, but does not free its memory.
 *     Does not deregister process with runtime.
 * msize() gives the size required to store a process.
 */
ndl_proc *ndl_proc_minit(void *region, ndl_runtime *runtime,
                         ndl_pid pid, ndl_ref local, ndl_time period);
void      ndl_proc_mkill(ndl_proc *proc);
int64_t   ndl_proc_msize(ndl_runtime *runtime,
                         ndl_ref local, ndl_time period);

/* Suspend or resume processes.
 *
 * suspend() deactivates a process.
 *     Returns 0 on success, nonzero on error.
 * resume() resumes a process.
 *     Returns 0 on success, nonzero on error.
 * active() gets the active/inactive state of the process.
 *     Returns 0 if suspended, 1 if active, -1 on error.
 */
int ndl_proc_suspend(ndl_proc *proc);
int ndl_proc_resume (ndl_proc *proc);
int ndl_proc_active (ndl_proc *proc);

/* Set process state.
 * Transitions are only valid if listed in the diagram
 * above. Artificially trigger a state transition event.
 * All return 0 on success, nonzero on error.
 *
 * cancel() moves waiting or sleeping processes to running.
 * wait() move a running process to waiting.
 * sleep() moves a running process to sleeping.
 * die() moves a process in any living state to dead.
 *     Sets the reason to ECAUSE_KILLED.
 */
int ndl_proc_cancel(ndl_proc *proc);
int ndl_proc_wait  (ndl_proc *proc, ndl_ref node);
int ndl_proc_sleep (ndl_proc *proc, ndl_time duration);
int ndl_proc_die   (ndl_proc *proc);

/* Execute a process.
 *
 * run() steps the process forward.
 *     Runs for at most the given number of steps.
 *     May remove process from event list, but event list's next PID will be valid.
 */
void ndl_proc_run(ndl_proc *proc, uint64_t steps);

/* Get and set process period.
 * Processes can be run at a set frequency. This
 * allows you to set the frequency at which a process runs,
 * represented as the period between cycles.
 *
 * period() gets the period at which this process runs.
 *     Returns NDL_TIME_ZERO for as-fast-as-possible.
 * setperiod() sets the period to the value.
 *     NDL_TIME_ZERO for as-fast-as-possible.
 *     Returns 0 on success, nonzero on error.
 */
ndl_time ndl_proc_period   (ndl_proc *proc);
int      ndl_proc_setperiod(ndl_proc *proc, ndl_time duration);

/* Get state related information.
 *
 * status() gets the current state of the process.
 *
 * waiting() gets the node the process is waiting on.
 *     Returns reference on success, NDL_NULL_REF on error/bad state.
 * sleeping() gets the remaining time on the clock.
 *     Returns NDL_TIME_ZERO if zero *or* error/bad state.
 * cause() gets the cause of death for the process.
 *     Returns ECAUSE_NULL on error, ECAUSE_NONE on bad state, else cause.
 */
ndl_proc_state  ndl_proc_status(ndl_proc *proc);

ndl_ref         ndl_proc_waiting (ndl_proc *proc);
ndl_time        ndl_proc_sleeping(ndl_proc *proc);
ndl_proc_reason ndl_proc_cause   (ndl_proc *proc);

/* Process information.
 *
 * pid() gets the process' PID.
 * local() gets the process' local node.
 *
 * runtime() gets the associated runtime.
 */
ndl_pid ndl_proc_pid  (ndl_proc *proc);
ndl_ref ndl_proc_local(ndl_proc *proc);

ndl_runtime *ndl_proc_runtime(ndl_proc *proc);

/* Print out a process. */
void ndl_proc_print(ndl_proc *proc);

#endif /* NODEL_PROC_H */
