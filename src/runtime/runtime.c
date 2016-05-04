#include "runtime.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

static void ndl_runtime_clockevent_swap(void *a, void *b) {

    ndl_runtime_clockevent *at = (ndl_runtime_clockevent *) a;
    ndl_runtime_clockevent *bt = (ndl_runtime_clockevent *) b;
    ndl_runtime_clockevent ct;

     ct = *at;
    *at = *bt;
    *bt =  ct;

    ndl_proc *aproc = ndl_runtime_proc(at->runtime, at->head);
    if (aproc != NULL)
        aproc->head = (void *) at;

    ndl_proc *bproc = ndl_runtime_proc(bt->runtime, bt->head);
    if (bproc != NULL)
        bproc->head = (void *) bt;
}

static int ndl_runtime_clockevent_cmp(void *a, void *b) {

    ndl_runtime_clockevent *at = (ndl_runtime_clockevent *) a;
    ndl_runtime_clockevent *bt = (ndl_runtime_clockevent *) b;

    return ndl_time_cmp(at->when, bt->when);
}

ndl_runtime *ndl_runtime_init(ndl_graph *graph) {

    ndl_runtime *rt = malloc(sizeof(ndl_runtime));

    if (rt == NULL)
        return NULL;

    ndl_runtime *res = ndl_runtime_minit(rt, graph);

    if (res == NULL) {
        free(rt);
        return NULL;
    }

    return res;
}

void ndl_runtime_kill(ndl_runtime *runtime) {

    ndl_runtime_mkill(runtime);

    free(runtime);

    return;
}

ndl_runtime *ndl_runtime_minit(void *region, ndl_graph *graph) {

    ndl_runtime *ret = region;

    if (ret == NULL)
        return NULL;

    /* Graph init. */
    if (graph == NULL) {
        ret->free_graph = 1;

        ret->graph = ndl_graph_init();
        if (ret->graph == NULL) {
            free(ret);
            return NULL;
        }
    } else {
        ret->free_graph = 0;

        ret->graph = graph;
    }

    /* Procs init. */
    ndl_rhashtable *procs = ndl_rhashtable_init(sizeof(ndl_pid), sizeof(ndl_proc), 64);
    if (procs == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        free(ret);

        return NULL;
    }
    ret->next_pid = (ndl_pid) 1;
    ret->procs = procs;

    /* Waitevents init. */
    ndl_rhashtable *waitevents = ndl_rhashtable_init(sizeof(ndl_ref), sizeof(ndl_pid), 8);
    if (waitevents == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        ndl_rhashtable_kill(procs);
        free(ret);

        return NULL;
    }
    ret->waitevents = waitevents;

    /* Clock event init. */
    ndl_heap *clockevents =
        ndl_heap_init(sizeof(ndl_runtime_clockevent),
                      &ndl_runtime_clockevent_cmp,
                      &ndl_runtime_clockevent_swap);
    if (clockevents == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        ndl_rhashtable_kill(procs);
        ndl_rhashtable_kill(waitevents);
        free(ret);

        return NULL;
    }
    ret->clockevents = clockevents;

    ndl_eval_opcodes_ref();

    return ret;
}

void ndl_runtime_mkill(ndl_runtime *runtime) {

    if (runtime == NULL)
        return;

    if (runtime->graph != NULL)
        if (runtime->free_graph == 1)
            ndl_graph_kill(runtime->graph);

    if (runtime->procs != NULL) ndl_rhashtable_kill(runtime->procs);
    if (runtime->waitevents != NULL) ndl_rhashtable_kill(runtime->waitevents);
    if (runtime->clockevents != NULL) ndl_heap_kill(runtime->clockevents);

    ndl_eval_opcodes_deref();
}

uint64_t ndl_runtime_msize(ndl_graph *graph) {

    return sizeof(ndl_runtime);
}

ndl_proc *ndl_runtime_proc(ndl_runtime *runtime, ndl_pid pid) {

    return (ndl_proc *) ndl_rhashtable_get(runtime->procs, &pid);
}

ndl_proc *ndl_runtime_proc_init(ndl_runtime *runtime,
                                ndl_ref local, ndl_time period) {

    ndl_pid pid = runtime->next_pid;
    void *region = ndl_rhashtable_put(runtime->procs, &pid, NULL);
    if (region == NULL)
        return NULL;

    ndl_proc *proc = ndl_proc_minit(region, runtime, pid, local, period);

    runtime->next_pid++;

    return proc;
}

int ndl_runtime_proc_kill(ndl_runtime *runtime, ndl_proc *proc) {

    int err = ndl_proc_suspend(proc);
    if (err != 0)
        return err;

    return ndl_rhashtable_del(runtime->procs, &proc->pid);
}

void *ndl_runtime_proc_head(ndl_runtime *runtime) {

    return ndl_rhashtable_pairs_head(runtime->procs);
}

void *ndl_runtime_proc_next(ndl_runtime *runtime, void *prev) {

    return ndl_rhashtable_pairs_next(runtime->procs, prev);
}

ndl_pid ndl_runtime_proc_pid(ndl_runtime *runtime, void *curr) {

    ndl_pid *res = (ndl_pid *) ndl_rhashtable_pairs_key(runtime->procs, curr);
    if (res == NULL)
        return NDL_NULL_PID;

    return *res;
}

ndl_proc *ndl_runtime_proc_proc(ndl_runtime *runtime, void *curr) {

    return (ndl_proc *) ndl_rhashtable_pairs_val(runtime->procs, curr);
}

ndl_graph *ndl_runtime_graph(ndl_runtime *runtime) {

    return runtime->graph;
}

int ndl_runtime_graph_free(ndl_runtime *runtime) {

    return runtime->free_graph;
}

uint64_t ndl_runtime_proc_count(ndl_runtime *runtime) {

    return ndl_rhashtable_size(runtime->procs);
}

uint64_t ndl_runtime_proc_living(ndl_runtime *runtime) {

    uint64_t total = ndl_runtime_proc_count(runtime);

    void *curr = ndl_rhashtable_pairs_head(runtime->procs);
    while (curr != NULL) {
        ndl_proc *proc = ndl_rhashtable_pairs_val(runtime->procs, curr);
        if (proc != NULL)
            if (!proc->active)
                total--;

        curr = ndl_rhashtable_pairs_next(runtime->procs, curr);
    }

    return total;
}

int ndl_runtime_proc_alive(ndl_runtime *runtime) {

    return (ndl_heap_size(runtime->clockevents) != 0)? 1 : 0;
}

static inline int ndl_runtime_run_event(ndl_runtime *runtime,
                                        ndl_runtime_clockevent *head) {

    ndl_pid pid = head->head;
    ndl_proc *curr = ndl_rhashtable_get(runtime->procs, &pid);
    if (curr == NULL)
        return -1;

    if (curr->state == ESTATE_SLEEPING)
        return ndl_proc_cancel(curr);

    ndl_time period = curr->period;

    ndl_pid next_pid;
    int count = 0;
    do {
        curr = ndl_rhashtable_get(runtime->procs, &pid);
        if (curr == NULL)
            return -1;

        next_pid = curr->event_next;

        ndl_proc_run(curr, 1); /* TODO: Increase? */

        count++;

        pid = next_pid;

    } while (pid != -1);

    head->when = ndl_time_add(head->when, period);
    ndl_heap_readj(runtime->clockevents, head);

    return count;
}


#define NDL_RUNTIME_MIN_SLEEP (ndl_time_from_usec(10))
#define NDL_RUNTIME_MIN_CYCLES 32

static inline int ndl_runtime_run_cready(ndl_runtime *runtime, ndl_time timeout, ndl_time now) {

    if (ndl_time_cmp(timeout, NDL_TIME_ZERO) == 0) {
        timeout.tv_sec = LONG_MAX;
        timeout.tv_nsec = 0;
    } else {
        timeout = ndl_time_add(now, timeout);
    }

    int rcount = 0;
    int err = 0;
    while (err >= 0) {
        ndl_runtime_clockevent *head = ndl_heap_peek(runtime->clockevents);
        if (head == NULL)
            return 0;

        if (ndl_time_cmp(ndl_time_sub(head->when, now), NDL_TIME_ZERO) < 0) {

            err = ndl_runtime_run_event(runtime, head);
            if (err < 0)
                return err;

            rcount += err;
        } else {

            return 0;
        }

        if (rcount > NDL_RUNTIME_MIN_CYCLES) {
            now = ndl_time_get();
            if (ndl_time_cmp(now, NDL_TIME_ZERO) == 0)
                return -1;
            rcount = 0;
        }
    }

    return err;
}

int ndl_runtime_run_ready(ndl_runtime *runtime, ndl_time timeout) {

    ndl_time start = ndl_time_get();
    if (ndl_time_cmp(start, NDL_TIME_ZERO) == 0)
        return -1;

    return ndl_runtime_run_cready(runtime, timeout, start);
}

static inline ndl_time ndl_runtime_run_ctimeto(ndl_runtime *runtime, ndl_time now) {

    ndl_runtime_clockevent *head = ndl_heap_peek(runtime->clockevents);
    if (head == NULL)
        return NDL_TIME_ZERO;

    now = ndl_time_sub(head->when, now);
    if (ndl_time_cmp(now, NDL_TIME_ZERO) < 0)
        return NDL_TIME_ZERO;

    return now;
}

static ndl_time ndl_runtime_run_csleep(ndl_runtime *runtime, ndl_time timeout, ndl_time now) {

    if (ndl_time_cmp(timeout, NDL_TIME_ZERO) == 0) {
        timeout.tv_sec = LONG_MAX;
        timeout.tv_nsec = 0;
    } else {
        timeout = ndl_time_add(now, timeout);
    }

    ndl_time timeto = ndl_runtime_run_ctimeto(runtime, now);
    if (ndl_time_cmp(timeto, NDL_TIME_ZERO) == 0)
        return NDL_TIME_ZERO;

    if (ndl_time_cmp(timeto, timeout) <= 0)
        timeout = timeto;

    int64_t usec = ndl_time_to_usec(timeout);
    int err;
    if (ndl_time_cmp(timeout, NDL_RUNTIME_MIN_SLEEP) >= 0)
        err = usleep((uint32_t) usec);
    else
        return NDL_TIME_ZERO;

    if (err != 0)
        return NDL_TIME_ZERO;

    ndl_time end = ndl_time_get();
    if (ndl_time_cmp(end, NDL_TIME_ZERO) == 0)
        return ndl_time_add(now, timeout); /* Guess. */

    return ndl_time_sub(end, now);
}

ndl_time ndl_runtime_run_sleep(ndl_runtime *runtime, ndl_time timeout) {

    ndl_time now = ndl_time_get();
    if (ndl_time_cmp(now, NDL_TIME_ZERO) == 0)
        return NDL_TIME_ZERO;

    return ndl_runtime_run_csleep(runtime, timeout, now);
}

ndl_time ndl_runtime_run_timeto(ndl_runtime *runtime) {

    ndl_time now = ndl_time_get();
    if (ndl_time_cmp(now, NDL_TIME_ZERO) == 0)
        return NDL_TIME_ZERO;

    return ndl_runtime_run_ctimeto(runtime, now);
}

int ndl_runtime_run_for(ndl_runtime *runtime, ndl_time timeout) {

    ndl_time curr = ndl_time_get();
    if (ndl_time_cmp(curr, NDL_TIME_ZERO) == 0)
        return -1;

    if (ndl_time_cmp(timeout, NDL_TIME_ZERO) == 0) {
        timeout.tv_sec = LONG_MAX;
        timeout.tv_nsec = 0;
    } else {

        timeout = ndl_time_add(curr, timeout);
    }

    ndl_time remaining = ndl_time_sub(timeout, curr);

    while (remaining.tv_sec >= 0) {

        int err = ndl_runtime_run_cready(runtime, remaining, curr);
        if (err < 0)
            return err;
        if (err == 1)
            break;

        if (!ndl_runtime_proc_alive(runtime))
            break;

        curr = ndl_time_get();
        if (ndl_time_cmp(curr, NDL_TIME_ZERO) == 0)
            return -1;

        remaining = ndl_time_sub(timeout, curr);

        if (remaining.tv_sec < 0)
            break;

        ndl_time slept = ndl_runtime_run_csleep(runtime, remaining, curr);
        if (ndl_time_cmp(slept, NDL_TIME_ZERO) == 0)
            return -1;

        curr = ndl_time_add(curr, slept);
        remaining = ndl_time_sub(timeout, curr);
    }

    return 0;
}

void ndl_runtime_print(ndl_runtime *runtime) {

    printf("Printing runtime.\n");
    ndl_rhashtable_print(runtime->procs);
    ndl_heap_print(runtime->clockevents);
}
