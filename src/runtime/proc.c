#include "proc.h"
#include "eval.h"

#include <stdio.h>
#include <stdlib.h>

ndl_proc *ndl_proc_minit(void *region, ndl_runtime *runtime,
                         ndl_pid pid, ndl_ref local, ndl_time period) {

    ndl_proc *proc = (ndl_proc *) region;
    if (proc == NULL)
        return NULL;

    proc->runtime = runtime;
    proc->pid = pid;

    proc->active = 0;
    proc->state = ESTATE_RUNNING;

    proc->event_prev = NDL_NULL_PID;
    proc->event_next = NDL_NULL_PID;

    proc->local = local;
    proc->period = period;

    return proc;
}

void ndl_proc_mkill(ndl_proc *proc) {

    return;
}

int64_t ndl_proc_msize(ndl_runtime *runtime, ndl_ref local, ndl_time period) {

    return sizeof(ndl_proc);
}

int ndl_proc_suspend(ndl_proc *proc) {

    if (!proc->active)
        return 0;

    switch (proc->state) {

    case ESTATE_RUNNING: break;
    case ESTATE_WAITING: break;
    case ESTATE_SLEEPING: break;
    default: break;
    }

    proc->active = 0;

    return 0;
}

int ndl_proc_resume(ndl_proc *proc) {

    if (proc->active)
        return 0;

    switch (proc->state) {

    case ESTATE_RUNNING: break;
    case ESTATE_WAITING: break;
    case ESTATE_SLEEPING: break;
    default: break;
    }

    proc->active = 1;

    return 0;
}

int ndl_proc_active(ndl_proc *proc) {

    return proc->active;
}

int ndl_proc_cancel(ndl_proc *proc) {

    if ((proc->state != ESTATE_WAITING) && (proc->state != ESTATE_SLEEPING) &&
        (proc->state != ESTATE_RUNNING))
        return -1;

    int err = 0;

    int hooked = proc->active;
    if (hooked)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return -1;

    proc->state = ESTATE_RUNNING;

    if (hooked)
        err = ndl_proc_resume(proc);

    return err;
}

int ndl_proc_wait(ndl_proc *proc, ndl_ref node) {

    if ((proc->state != ESTATE_WAITING) && (proc->state != ESTATE_RUNNING))
        return -1;

    int err = 0;

    int hooked = proc->active;
    if (hooked)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return -1;

    proc->state = ESTATE_WAITING;
    proc->waiting = node;

    if (hooked)
        err = ndl_proc_resume(proc);

    return err;
}

int ndl_proc_sleep(ndl_proc *proc, ndl_time duration) {

    if ((proc->state != ESTATE_SLEEPING) && (proc->state != ESTATE_RUNNING))
        return -1;

    int err = 0;

    int hooked = proc->active;
    if (hooked)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return -1;

    proc->state = ESTATE_SLEEPING;
    proc->duration = duration;

    if (hooked)
        err = ndl_proc_resume(proc);

    return err;
}

int ndl_proc_die(ndl_proc *proc) {

    int err = 0;

    if ((proc->state != ESTATE_DEAD) && proc->active)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return err;

    proc->state = ESTATE_DEAD;
    proc->cause_of_death = ECAUSE_KILLED;

    return 0;
}

static int ndl_proc_die_reason(ndl_proc *proc, ndl_proc_reason reason) {

    int err = 0;

    if ((proc->state != ESTATE_DEAD) && proc->active)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return err;

    proc->state = ESTATE_DEAD;
    proc->cause_of_death = reason;

    return 0;
}

static ndl_proc_reason ndl_proc_excall(ndl_proc *proc, ndl_eval_result res) {

    ndl_ref inst = res.actval.ref;
    ndl_value val = ndl_graph_get(proc->runtime->graph, inst, NDL_SYM("syma    "));
    if ((val.type != EVAL_SYM) || (val.sym == NDL_NULL_SYM))
        return ECAUSE_BAD_DATA;

    ndl_excall *table = ndl_eval_excall();
    if (table == NULL)
        return ECAUSE_INTERNAL;

    ndl_excall_func func = ndl_excall_get(table, val.sym);
    if (func == NULL)
        return ECAUSE_BAD_DATA;

    if (func(inst, proc->local, (void *) proc))
        return ECAUSE_NULL;

    return ECAUSE_NONE;
}

static void ndl_proc_checkmod(ndl_runtime *runtime, ndl_ref node) {

    void *wait = (ndl_proc *) ndl_rhashtable_get(runtime->waitevents, &node);
    if (wait == NULL)
        return;

    ndl_pid next = *((ndl_pid *) wait);

    while (next != NDL_NULL_PID) {

        ndl_proc *proc = (ndl_proc *) ndl_rhashtable_get(runtime->procs, &next);
        if (proc == NULL)
            return;

        next = proc->event_next;

        int err = ndl_proc_suspend(proc);
        if (err != 0)
            return;

        proc->state = ESTATE_RUNNING;

        err = ndl_proc_resume(proc);
        if (err != 0)
            return;
    }
}

static void ndl_proc_step(ndl_proc *proc) {

    ndl_graph *graph = proc->runtime->graph;
    ndl_ref local = proc->local;

    ndl_eval_result res = ndl_eval(graph, local);

    ndl_proc_reason reason = ECAUSE_NONE;

    int err;
    ndl_proc *nproc;
    switch (res.action) {
    case EACTION_CALL:

        if (res.actval.type != EVAL_REF || res.actval.ref == NDL_NULL_REF) {
            reason = ECAUSE_BAD_DATA;
        } else {
            ndl_graph_unmark(graph, local);
            ndl_graph_mark(graph, res.actval.ref);
            proc->local = res.actval.ref;
        }

        break;

    case EACTION_FORK:

        if ((res.actval.type != EVAL_REF) || (res.actval.ref == NDL_NULL_REF)) {
            reason = ECAUSE_BAD_DATA;
        } else {
            nproc = ndl_runtime_proc_init(proc-> runtime, res.actval.ref, proc->period);
            if (nproc == NULL) {
                reason = ECAUSE_INTERNAL;
                break;
            }

            err = ndl_proc_resume(nproc);
            if (err != 0) {
                reason = ECAUSE_INTERNAL;
                break;
            }
        }

        break;

    case EACTION_NONE:
        break;

    case EACTION_EXIT:
        reason = ECAUSE_EXIT;
        break;

    case EACTION_WAIT:
        if ((res.actval.type != EVAL_REF) || (res.actval.ref == NDL_NULL_REF)) {
            reason = ECAUSE_BAD_DATA;
        } else {
            err = ndl_proc_wait(proc, res.actval.ref);
            if (err != 0)
                reason = ECAUSE_INTERNAL;
        }
        break;

    case EACTION_SLEEP:
        if ((res.actval.type != EVAL_INT) || (res.actval.num < 0)) {
            reason = ECAUSE_BAD_DATA;
        } else {
            err = ndl_proc_sleep(proc, ndl_time_from_usec(1000 * res.actval.num));
            if (err != 0)
                reason = ECAUSE_INTERNAL;
        }
        break;

    case EACTION_EXCALL:
        reason = ndl_proc_excall(proc, res);
        break;

    case EACTION_FAIL:
    default:
        reason = ECAUSE_BAD_INST;
        break;
    }

    int i;
    for (i = 0; i < res.mod_count; i++)
        ndl_proc_checkmod(proc->runtime, res.mod[i]);

    if (reason != ECAUSE_NONE)
        ndl_proc_die_reason(proc, reason);

    return;
}

void ndl_proc_run(ndl_proc *proc, uint64_t steps) {

    while (steps-- > 0) {

        if (!proc->active || (proc->state != ESTATE_RUNNING))
            break;

        ndl_proc_step(proc);
    }
}


ndl_time ndl_proc_period(ndl_proc *proc) {

    return proc->period;
}

int ndl_proc_setperiod(ndl_proc *proc, ndl_time duration) {

    int err = 0;

    int running = (proc->state == ESTATE_RUNNING) && proc->active;
    if (running)
        err = ndl_proc_suspend(proc);

    if (err != 0)
        return err;

    proc->duration = duration;

    if (running)
        err = ndl_proc_resume(proc);

    return err;
}

ndl_proc_state ndl_proc_status(ndl_proc *proc) {

    return proc->state;
}

ndl_ref ndl_proc_waiting(ndl_proc *proc) {

    if (proc->state != ESTATE_WAITING)
        return NDL_NULL_REF;

    return proc->waiting;
}

ndl_time ndl_proc_sleeping(ndl_proc *proc) {

    if (proc->state != ESTATE_SLEEPING)
        return NDL_TIME_ZERO;

    if (proc->active == 0)
        return proc->duration;

    ndl_time curr = ndl_time_get();
    if (ndl_time_cmp(curr, NDL_TIME_ZERO) == 0)
        return curr;

    ndl_time left = ndl_time_sub(proc->duration, curr);
    if (ndl_time_cmp(left, NDL_TIME_ZERO) <= 0)
        return NDL_TIME_ZERO;

    return left;
}

ndl_proc_reason ndl_proc_cause(ndl_proc *proc) {

    if (proc->state != ESTATE_DEAD)
        return ECAUSE_NONE;

    return proc->cause_of_death;
}

ndl_pid ndl_proc_pid(ndl_proc *proc) {

    return proc->pid;
}

ndl_ref ndl_proc_local(ndl_proc *proc) {

    return proc->local;
}

ndl_runtime *ndl_proc_runtime(ndl_proc *proc) {

    return proc->runtime;
}

static inline const char *ndl_proc_print_state(ndl_proc_state state) {

    switch (state) {

    case ESTATE_RUNNING: return "RUNNING";
    case ESTATE_SLEEPING: return "SLEEPING";
    case ESTATE_WAITING: return "WAITING";
    case ESTATE_DEAD: return "DEAD";
    default:
    case ESTATE_NULL: return "NULL";
    }
}

static inline const char *ndl_proc_print_reason(ndl_proc_reason cause) {

    switch (cause) {

    case ECAUSE_NONE: return "None";
    case ECAUSE_EXIT: return "Exit()";
    case ECAUSE_KILLED: return "Killed";
    case ECAUSE_BAD_INST: return "Bad instruction";
    case ECAUSE_BAD_DATA: return "Bad arguments";
    case ECAUSE_INTERNAL: return "Internal error";
    default:
    case ECAUSE_NULL: return "NULL";
    }
}

void ndl_proc_print(ndl_proc *proc) {

    fprintf(stderr, "[%03ld] Printing proc:\n", proc->pid);

    fprintf(stderr, "[%03ld] State: %s|%s\n", proc->pid,
            (proc->active)? "active":"inactive",
            ndl_proc_print_state(proc->state));

    fprintf(stderr, "[%03ld] Local: %03ld\n", proc->pid, proc->local);

    fprintf(stderr, "[%03ld] Period:\n", proc->pid);
    ndl_time_print(proc->period);

    switch (proc->state) {
    case ESTATE_SLEEPING:
        if (proc->active)
            fprintf(stderr, "[%03ld] Sleeping until:\n", proc->pid);
        else
            fprintf(stderr, "[%03ld] Time left:\n", proc->pid);
        ndl_time_print(proc->duration);
        break;

    case ESTATE_WAITING:
        fprintf(stderr, "[%03ld] Waiting on node %03ld.\n", proc->pid, proc->waiting);
        break;

    case ESTATE_DEAD:
        fprintf(stderr, "[%03ld] Cause of death: %s.\n", proc->pid,
                ndl_proc_print_reason(proc->cause_of_death));
        break;
    default:
        break;
    }
}
