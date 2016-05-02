#ifndef NODEL_PROC_H
#define NODEL_PROC_H

#include "node.h"

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
    ESTATE_WATIING,

    ESTATE_DEAD,

    ESTATE_SIZE
} ndl_proc_state;

typedef enum ndl_proc_cause_of_death_e {

    ECAUSE_NULL,     /* Process does not exist. */

    ECAUSE_NONE,     /* Process is not dead. */

    ECAUSE_EXIT,     /* Process called exit(). */
    ECAUSE_KILLED,   /* Process was killed externally. */
    ECAUSE_BAD_INST, /* Process encountered bad instruction. */
    ECAUSE_BAD_DATA, /* Process encountered bad data. */

    ECAUSE_SIZE

} ndl_proc_cause_of_death;

typedef int64_t ndl_pid;
#define NDL_NULL_PROC ((ndl_pid) -1)

typedef struct ndl_proc_s {

    /* General state */
    ndl_pid pid;

    int active;
    ndl_proc_state state;

    /* Event list. */
    ndl_pid event_prev, event_next;

    /* Local frame / process data. */
    ndl_ref local;   /* Local frame. */
    ndl_time period; /* Period between cycles (1/freq.) NDL_TIME_ZERO for unlimited. */

    /* State specific data. */
    union {

        /* Sleeping. */
        ndl_time duration; /* How much time left before waking up. */

        /* Waiting. */
        ndl_ref waiting; /* Node to wait on. */

        /* Dead. */
        ndl_proc_cause_of_death cause_of_death; /* What killed us? */
    };

} ndl_proc;

#endif /* NODEL_PROC_H */
