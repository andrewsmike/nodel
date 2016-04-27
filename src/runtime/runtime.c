#include "runtime.h"
#include "eval.h"
#include "rehashtable.h"
#include "slabheap.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <time.h>

/* A Nodel runtime.
 * Stores a couple different structures.
 *
 * Events are things that happen after some trigger.
 * Events all link to a doubly linked PID list for wakeups.
 * There are two types of events.
 *
 * The first type is clock events.
 * The runtime has a heap of clock events, ordered by start time. These include
 * calls to sleep and running processes sleeping between cycles (when freq is limited.)
 *
 * The second type is wait based events.
 * Processes may 'wait' for a node to be modified.
 * These events are stored in a (resizable) hashtable mapping
 * node ID -> PID list. When a node is modified, all PIDs waiting
 * on it are moved to their various frequency events.
 */
typedef struct ndl_clock_event_s {

    struct timespec when;

    int64_t head_pid;

} ndl_clock_event;

typedef struct ndl_process_s {

    /* Process state and event list. */
    ndl_proc_state state;

    union {
        ndl_clock_event *sleeping;
        ndl_ref waiting;
    } event_group;
    int64_t event_prev_pid, event_next_pid;

    uint64_t freq;

    /* Metadata. */

    int64_t pid;
    ndl_ref local;

} ndl_process;

struct ndl_runtime_s {

    int free_graph;
    ndl_graph *graph;

    /* Map from pid -> process. Don't hold pointers, rhashtable moves. */
    int64_t next_pid;
    ndl_rhashtable *procs;

    /* Heap for clock events. */
    ndl_slabheap *cevents;

    /* Map from frequency to clock event. */
    ndl_rhashtable *freqs;

    /* Map from ndl_ref -> PID list head. */
    ndl_rhashtable *waits;
};

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


static inline int ndl_runtime_run_cmp(struct timespec a, struct timespec b) {

    if (a.tv_sec < b.tv_sec)
        return -1;
    else if (a.tv_sec > b.tv_sec)
        return 1;
    else if (a.tv_nsec < b.tv_nsec)
        return -1;
    else if (a.tv_nsec > b.tv_nsec)
        return 1;

    return 0;
}

static int ndl_clock_event_cmp(void *a, void *b) {

    ndl_clock_event *ca = (ndl_clock_event *) a;
    ndl_clock_event *cb = (ndl_clock_event *) b;

    return ndl_runtime_run_cmp(ca->when, cb->when);
}

ndl_runtime *ndl_runtime_minit(void *region, ndl_graph *graph) {

    ndl_runtime *ret = region;

    if (ret == NULL)
        return NULL;

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

    ndl_rhashtable *procs = ndl_rhashtable_init(sizeof(int64_t), sizeof(ndl_process), 16);
    if (procs == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        free(ret);

        return NULL;
    }
    ret->next_pid = 0;
    ret->procs = procs;


    ndl_rhashtable *freqs = ndl_rhashtable_init(sizeof(uint64_t), sizeof(ndl_clock_event *), 8);
    if (freqs == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        ndl_rhashtable_kill(procs);
        free(ret);

        return NULL;
    }
    ret->freqs = freqs;


    ndl_rhashtable *waits = ndl_rhashtable_init(sizeof(ndl_ref), sizeof(int64_t), 4);
    if (waits == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        ndl_rhashtable_kill(procs);
        ndl_rhashtable_kill(freqs);
        free(ret);

        return NULL;
    }
    ret->waits = waits;

    ndl_slabheap *cevents = ndl_slabheap_init(sizeof(ndl_clock_event), &ndl_clock_event_cmp, 16);
    if (cevents == NULL) {
        if (ret->free_graph == 1)
            ndl_graph_kill(ret->graph);
        ndl_rhashtable_kill(procs);
        ndl_rhashtable_kill(freqs);
        ndl_rhashtable_kill(waits);
        free(ret);

        return NULL;
    }
    ret->cevents = cevents;

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
    if (runtime->freqs != NULL) ndl_rhashtable_kill(runtime->freqs);
    if (runtime->waits != NULL) ndl_rhashtable_kill(runtime->waits);
    if (runtime->cevents != NULL) ndl_slabheap_kill(runtime->cevents);

    ndl_eval_opcodes_deref();
}

uint64_t ndl_runtime_msize(ndl_graph *graph) {

    return sizeof(ndl_runtime);
}

/* Struct timespec helpers. */

#define GIGA 1000000000
#define MEGA 1000000
#define KILO 1000

static inline struct timespec ndl_runtime_run_ts_sub(struct timespec a, struct timespec b) {

    struct timespec ret;
    ret.tv_sec = a.tv_sec - b.tv_sec;
    ret.tv_nsec = a.tv_nsec - b.tv_nsec;

    if (ret.tv_nsec < 0) {
        ret.tv_nsec += GIGA;
        ret.tv_sec--;
    }

    return ret;
}

static inline struct timespec ndl_runtime_run_ts_add(struct timespec a, struct timespec b) {

    struct timespec ret;
    ret.tv_sec = a.tv_sec + b.tv_sec;
    ret.tv_nsec = a.tv_nsec + b.tv_nsec;

    if (ret.tv_nsec > GIGA) {
        ret.tv_nsec -= GIGA;
        ret.tv_sec++;
    }

    return ret;
}

static inline struct timespec ndl_runtime_run_usec_to_ts(int64_t interval) {

    struct timespec ret;
    ret.tv_sec = interval / MEGA;
    ret.tv_nsec = interval % MEGA;

    if (ret.tv_nsec < 0) {
        ret.tv_nsec += GIGA;
        ret.tv_sec--;
    }

    return ret;
}

static inline int64_t ndl_runtime_run_ts_to_usec(struct timespec interval) {

    int64_t ret = 0;
    ret += interval.tv_sec * MEGA;
    ret += interval.tv_nsec / KILO;

    return ret;
}

/* helpers for each process state transition. */

/* suspended -> running */
static inline int ndl_runtime_freq_add(ndl_runtime *runtime, int64_t pid, uint64_t freq) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    ndl_process *head = NULL;

    ndl_clock_event *event = ndl_rhashtable_get(runtime->freqs, &freq);
    if (event == NULL) {

        ndl_clock_event cevent;
        cevent.when = ndl_runtime_run_usec_to_ts((int64_t) 0);
        cevent.head_pid = -1;

        event = ndl_slabheap_put(runtime->cevents, &cevent);
        if (event == NULL)
            return -1;

        ndl_clock_event *t = ndl_rhashtable_put(runtime->freqs, &freq, &event);
        if (t == NULL) {
            ndl_slabheap_del(runtime->cevents, event);
            return -1;
        }
    } else {

        head = ndl_rhashtable_get(runtime->procs, &event->head_pid);
        if (head == NULL)
            return -1;
    }

    if (head) head->event_prev_pid = pid;
    proc->event_next_pid = event->head_pid;
    proc->event_prev_pid = -1;
    proc->state = ESTATE_RUNNING;
    proc->event_group.sleeping = event;
    event->head_pid = pid;

    return 0;
}

/* running -> suspended */
static inline int ndl_runtime_freq_del(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    ndl_process *prev = NULL;
    if (proc->event_prev_pid != -1) {
        prev = ndl_rhashtable_get(runtime->procs, &proc->event_prev_pid);
        if (prev == NULL)
            return -1;
    }

    ndl_process *next = NULL;
    if (proc->event_next_pid != -1) {
        next = ndl_rhashtable_get(runtime->procs, &proc->event_next_pid);
        if (next == NULL)
            return -1;
    }

    ndl_clock_event *event = proc->event_group.sleeping;

    if ((prev == NULL) && (next == NULL)) {

        int err = ndl_rhashtable_del(runtime->freqs, &proc->freq);
        if (err != 0)
            return err;

        err = ndl_slabheap_del(runtime->cevents, event);
        if (err != 0)
            return err;
    }

    if ((prev == NULL) && (next != NULL))
        event->head_pid = proc->event_next_pid;

    if (prev != NULL)
        prev->event_prev_pid = proc->event_next_pid;
    if (next != NULL)
        next->event_prev_pid = proc->event_prev_pid;

    proc->event_group.sleeping = NULL;
    proc->event_prev_pid = proc->event_next_pid = -1;
    proc->state = ESTATE_SUSPENDED;

    return 0;
}

/* suspended -> sleeping */
static inline int ndl_runtime_sleep_add(ndl_runtime *runtime, int64_t pid, uint64_t interval) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    ndl_clock_event cevent;
    int err = clock_gettime(CLOCK_BOOTTIME, &cevent.when);
    if (err != 0)
        return err;

    cevent.when = ndl_runtime_run_ts_sub(cevent.when, ndl_runtime_run_usec_to_ts((int64_t) interval));

    cevent.head_pid = pid;

    void *event = ndl_slabheap_put(runtime->cevents, &cevent);
    if (event == NULL)
        return -1;

    proc->state = ESTATE_SLEEPING;
    proc->event_group.sleeping = event;
    proc->event_prev_pid = proc->event_next_pid = -1;

    return 0;
}

/* sleeping -> suspended */
static inline int ndl_runtime_sleep_del(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    ndl_clock_event *node = proc->event_group.sleeping;
    int err = ndl_slabheap_del(runtime->cevents, node);
    if (err != 0)
        return err;

    proc->event_group.sleeping = NULL;
    proc->event_prev_pid = proc->event_next_pid = -1;
    proc->state = ESTATE_SUSPENDED;

    return 0;
}

/* suspended -> waiting */
static inline int ndl_runtime_wait_add(ndl_runtime *runtime, int64_t pid, ndl_ref node) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    int64_t *listhead = ndl_rhashtable_get(runtime->waits, &node);
    if (listhead == NULL) {

        listhead = ndl_rhashtable_put(runtime->waits, &node, &pid);
        if (listhead == NULL)
            return -1;

        proc->event_next_pid = -1;

    } else {

        int64_t oldpid = *listhead;

        ndl_process *oldproc = ndl_rhashtable_get(runtime->procs, &oldpid);
        if (oldproc == NULL)
            return -1;

        *listhead = pid;

        proc->event_next_pid = oldpid;
        oldproc->event_prev_pid = pid;
    }

    proc->event_prev_pid = -1;
    proc->event_group.waiting = node;
    proc->state = ESTATE_WAITING;

    return 0;
}

/* waiting -> suspended */
static inline int ndl_runtime_wait_del(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    ndl_process *prev = NULL;
    ndl_process *next = NULL;

    if (proc->event_prev_pid != -1) {

        prev = ndl_rhashtable_get(runtime->waits, &proc->event_prev_pid);
        if (prev == NULL)
            return -1;
    }

    if (proc->event_next_pid != -1) {

        next = ndl_rhashtable_get(runtime->waits, &proc->event_next_pid);
        if (next == NULL)
            return -1;
    }

    if (prev == NULL) {
        int64_t *listhead = ndl_rhashtable_get(runtime->waits, &proc->event_group.waiting);
        if (listhead == NULL)
            return 0;

        *listhead = proc->event_next_pid;
        if (*listhead == -1) {

            int err = ndl_rhashtable_del(runtime->waits, &proc->event_group.waiting);
            if (err != 0) {
                *listhead = pid;
                return err;
            }
        }
    } else {

        prev->event_next_pid = proc->event_next_pid;
    }

    if (next != NULL) {

        next->event_prev_pid = proc->event_prev_pid;
    }

    proc->event_group.sleeping = NULL;
    proc->event_prev_pid = proc->event_next_pid = -1;
    proc->state = ESTATE_SUSPENDED;

    return 0;
}

int ndl_runtime_proc_suspend(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    switch (proc->state) {
    default:
    case ESTATE_NONE:

        return -1;

    case ESTATE_SUSPENDED:

        return 0;

    case ESTATE_RUNNING:

        return ndl_runtime_freq_del(runtime, pid);

    case ESTATE_SLEEPING:

        return ndl_runtime_sleep_del(runtime, pid);

    case ESTATE_WAITING:

        return ndl_runtime_wait_del(runtime, pid);
    }
}

int ndl_runtime_proc_resume(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    /* For safety. */
    int err = ndl_runtime_proc_suspend(runtime, pid);
    if (err != 0)
        return err;

    return ndl_runtime_freq_add(runtime, pid, proc->freq);
}

int ndl_runtime_proc_setfreq(ndl_runtime *runtime, int64_t pid, uint64_t freq) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    if (proc->state != ESTATE_RUNNING) {
        proc->freq = freq;
        return 0;
    }

    int err = ndl_runtime_proc_suspend(runtime, pid);
    if (err != 0)
        return err;

    proc->freq = freq;

    return ndl_runtime_proc_resume(runtime, pid);
}

int64_t ndl_runtime_proc_getfreq(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return -1;

    return (int64_t) proc->freq;
}

int ndl_runtime_proc_setsleep(ndl_runtime *runtime, int64_t pid, uint64_t interval) {

    int err = ndl_runtime_proc_suspend(runtime, pid);
    if (err != 0)
        return err;

    return ndl_runtime_sleep_add(runtime, pid, interval);
}

int ndl_runtime_proc_setwait(ndl_runtime *runtime, int64_t pid, ndl_ref node) {

    int err = ndl_runtime_proc_suspend(runtime, pid);
    if (err != 0)
        return err;

    return ndl_runtime_wait_add(runtime, pid, node);
}

static inline int ndl_runtime_run_checkmod(ndl_runtime *runtime, ndl_ref ref) {

    int64_t *headp = ndl_rhashtable_get(runtime->waits, &ref);
    if (headp == NULL)
        return 0;

    int64_t head = *headp;

    ndl_process *curr = ndl_rhashtable_get(runtime->procs, &head);
    if (curr == NULL)
        return -1;

    while (curr != NULL) {

        int64_t next_pid = curr->event_next_pid;

        int err = ndl_runtime_proc_suspend(runtime, curr->pid);
        if (err != 0)
            return err;

        err = ndl_runtime_proc_resume(runtime, curr->pid);
        if (err != 0)
            return err;

        if (next_pid != -1)
            curr = ndl_rhashtable_get(runtime->procs, &next_pid);
        else
            curr = NULL;
    }

    return 0;
}

static void ndl_runtime_tick(ndl_runtime *runtime, int64_t pid) {

    ndl_process *proc = ndl_rhashtable_get(runtime->procs, &pid);
    if (proc == NULL)
        return;

    ndl_ref local = proc->local;
    ndl_graph *graph = runtime->graph;

    ndl_eval_result res = ndl_eval(graph, local);

    int exit = 0;

    int64_t npid;
    int err;
    switch (res.action) {
    case EACTION_CALL:

        if (res.actval.type != EVAL_REF || res.actval.ref == NDL_NULL_REF) {
            exit = 3;
        } else {
            ndl_graph_unmark(graph, local);
            ndl_graph_mark(graph, res.actval.ref);
            proc->local = res.actval.ref;
        }

        break;

    case EACTION_FORK:

        if ((res.actval.type != EVAL_REF) || (res.actval.ref == NDL_NULL_REF)) {
            exit = 3;
        } else {
            npid = ndl_runtime_proc_init(runtime, res.actval.ref);
            if (npid < 0) {
                exit = 2;
                break;
            }

            err = ndl_runtime_proc_setfreq(runtime, npid, proc->freq);
            if (err != 0) {
                exit = 2;
                break;
            }

            err = ndl_runtime_proc_resume(runtime, npid);
            if (err != 0) {
                exit = 2;
                break;
            }
        }

        break;

    case EACTION_NONE:
        break;

    case EACTION_EXIT:
        exit = 1;
        break;

    case EACTION_WAIT:
        if ((res.actval.type != EVAL_REF) || (res.actval.ref == NDL_NULL_REF)) {
            exit = 3;
        } else {
            err = ndl_runtime_proc_setwait(runtime, pid, res.actval.ref);
            if (err != 0)
                exit = 2;
        }
        break;

    case EACTION_SLEEP:
        if ((res.actval.type != EVAL_INT) || (res.actval.num < 0)) {
            exit = 3;
        } else {
            err = ndl_runtime_proc_setsleep(runtime, pid, (uint64_t) res.actval.num);
            if (err != 0)
                exit = 2;
        }
        break;

    case EACTION_EXCALL:
        exit = 2;
        break;

    case EACTION_FAIL:
    default:
        exit = 3;
        break;
    }

    int i;
    for (i = 0; i < res.mod_count; i++) {

        err = ndl_runtime_run_checkmod(runtime, res.mod[i]);
        if (err != 0)
            printf("[%3ld@%03ld] Failed to revive processes. God rest ye souls.\n", pid, local);
    }

    if (exit == 2)
        printf("[%3ld@%03ld] Process used excalls. Process destroyed itself in its hubris.\n", pid, local);
    if (exit == 3)
        printf("[%3ld@%03ld] Invalid local or bad instruction. We couldn't save 'em.\n", pid, local);

    if (exit != 0) {
        err = ndl_runtime_proc_kill(runtime, pid);
        if (err != 0)
            printf("[%3ld@%03ld] Failed to kill process. Here be zombies.\n", pid, local);
    }
}

#define min(a, b) ((a < b)? a : b)

#define NDL_RUNTIME_RUN_RESOLUTION 20

static inline int ndl_runtime_run_event(ndl_runtime *runtime, ndl_clock_event *head) {

    int64_t pid = head->head_pid;
    ndl_process *curr = ndl_rhashtable_get(runtime->procs, &pid);
    if (curr == NULL)
        return -1;

    if (curr->state == ESTATE_SLEEPING)
        return ndl_runtime_proc_resume(runtime, pid);

    int64_t freq = (int64_t) curr->freq;

    int64_t next_pid;
    int count = 0;
    do {
        curr = ndl_rhashtable_get(runtime->procs, &pid);
        if (curr == NULL)
            return -1;

        next_pid = curr->event_next_pid;

        ndl_runtime_tick(runtime, pid);

        count++;

        pid = next_pid;

    } while (pid != -1);

    head->when = ndl_runtime_run_ts_add(head->when,
                                        ndl_runtime_run_usec_to_ts(freq));
    ndl_slabheap_readj(runtime->cevents, head);

    return count;
}

static inline int ndl_runtime_run_cready(ndl_runtime *runtime, int64_t timeout, struct timespec start) {

    struct timespec end = ndl_runtime_run_usec_to_ts(timeout);
    end = ndl_runtime_run_ts_add(end, start);

    int rcount = 0;
    int err = 0;
    while (err >= 0) {
        ndl_clock_event *head = ndl_slabheap_peek(runtime->cevents);
        if (head == NULL)
            return 0;

        if (ndl_runtime_run_ts_sub(head->when, start).tv_sec < 0) {

            err = ndl_runtime_run_event(runtime, head);
            if (err < 0)
                return err;

            rcount += err;
        } else {

            return 0;
        }

        if (rcount > NDL_RUNTIME_RUN_RESOLUTION) {
            err = clock_gettime(CLOCK_BOOTTIME, &start);
            rcount = 0;
        }
    }

    return err;
}

int ndl_runtime_run_ready(ndl_runtime *runtime, int64_t timeout) {

    struct timespec start;
    int err = clock_gettime(CLOCK_BOOTTIME, &start);
    if (err != 0)
        return err;

    return ndl_runtime_run_cready(runtime, timeout, start);
}

static inline int64_t ndl_runtime_run_ctimeto(ndl_runtime *runtime, struct timespec now) {

    ndl_clock_event *head = ndl_slabheap_peek(runtime->cevents);
    if (head == NULL)
        return -1;

    now = ndl_runtime_run_ts_sub(head->when, now);
    if (now.tv_sec < 0)
        return 0;

    return ndl_runtime_run_ts_to_usec(now);
}

#define NDL_RUNTIME_RUN_MIN_SLEEP_USEC 100

static int64_t ndl_runtime_run_csleep(ndl_runtime *runtime, int64_t timeout, struct timespec start) {

    int64_t timeto = ndl_runtime_run_ctimeto(runtime, start);
    if (timeto < 0)
        return -1;

    if (timeto == 0)
        return 0;

    if (timeout <= 0) timeout = timeto;

    int err;
    uint32_t time = (uint32_t) min((uint64_t) timeto, (uint64_t) timeout);
    if (time > NDL_RUNTIME_RUN_MIN_SLEEP_USEC)
        err = usleep(time);
    else
        return 0;

    if (err != 0)
        return err;

    struct timespec end;
    err = clock_gettime(CLOCK_BOOTTIME, &end);
    if (err != 0)
        return err;

    end = ndl_runtime_run_ts_sub(end, start);

    return (int64_t) ndl_runtime_run_ts_to_usec(end);
}

int64_t ndl_runtime_run_sleep(ndl_runtime *runtime, int64_t timeout) {

    struct timespec start;
    int err = clock_gettime(CLOCK_BOOTTIME, &start);
    if (err != 0)
        return err;

    return ndl_runtime_run_csleep(runtime, timeout, start);
}

int64_t ndl_runtime_run_timeto(ndl_runtime *runtime) {

    struct timespec now;
    int err = clock_gettime(CLOCK_BOOTTIME, &now);
    if (err != 0)
        return err;

    return ndl_runtime_run_ctimeto(runtime, now);
}

int ndl_runtime_run_for(ndl_runtime *runtime, int64_t timeout) {

    struct timespec curr;
    int err = clock_gettime(CLOCK_BOOTTIME, &curr);
    if (err != 0)
        return err;

    struct timespec end = ndl_runtime_run_usec_to_ts(timeout);
    if (end.tv_sec >= 0) {
        end = ndl_runtime_run_ts_add(curr, end);
    } else {
        end.tv_sec = LONG_MAX;
        end.tv_nsec = 0;
    }

    struct timespec diff = ndl_runtime_run_ts_sub(end, curr);

    while (diff.tv_sec >= 0) {

        err = ndl_runtime_run_cready(runtime, ndl_runtime_run_ts_to_usec(diff), curr);
        if (err != 0)
            return err;

        if (ndl_runtime_dead(runtime) != 0)
            return 0;

        int err = clock_gettime(CLOCK_BOOTTIME, &curr);
        if (err != 0)
            return err;

        diff = ndl_runtime_run_ts_sub(end, curr);

        if (diff.tv_sec < 0)
            return 0;

        int64_t maxtime = ndl_runtime_run_ts_to_usec(diff);

        int64_t time = ndl_runtime_run_csleep(runtime, maxtime, curr);
        if (time == -1)
            return -1;

        diff = ndl_runtime_run_usec_to_ts(time);
        curr = ndl_runtime_run_ts_add(curr, diff);
        diff = ndl_runtime_run_ts_sub(end, curr);
    }

    return 0;
}

#define NDL_RUNTIME_FREQ_DEFAULT 10
int64_t ndl_runtime_proc_init(ndl_runtime *runtime, ndl_ref local) {

    int64_t pid = runtime->next_pid++;

    ndl_process procbase;

    procbase.event_group.waiting = NDL_NULL_REF;
    procbase.event_prev_pid = procbase.event_next_pid = -1;
    procbase.pid = pid;
    procbase.state = ESTATE_SUSPENDED;
    procbase.local = local;
    procbase.freq = NDL_RUNTIME_FREQ_DEFAULT;
    
    ndl_process *proc = ndl_rhashtable_put(runtime->procs, &pid, &procbase);
    if (proc == NULL) {
        runtime->next_pid--;
        return -1;
    }

    return pid;
}

int ndl_runtime_proc_kill(ndl_runtime *runtime, int64_t pid) {

    int err = ndl_runtime_proc_suspend(runtime, pid);
    if (err != 0)
        return err;

    return ndl_rhashtable_del(runtime->procs, &pid);
}

int64_t *ndl_runtime_proc_head(ndl_runtime *runtime) {

    /* TODO: Replace runtime.
    return (int64_t *) ndl_rhashtable_keyhead(runtime->procs);
    */
    return NULL;
}

int64_t *ndl_runtime_proc_next(ndl_runtime *runtime, int64_t *last) {

    /* TODO: Replace runtime.
    return (int64_t *) ndl_rhashtable_keynext(runtime->procs, (void *) last);
    */
    return NULL;
}

ndl_graph *ndl_runtime_graph(ndl_runtime *runtime) {

    return runtime->graph;
}

int ndl_runtime_graphown(ndl_runtime *runtime) {

    return runtime->free_graph;
}

uint64_t ndl_runtime_proc_count(ndl_runtime *runtime) {

    return ndl_rhashtable_size(runtime->procs);
}

int ndl_runtime_dead(ndl_runtime *runtime) {

    return (ndl_slabheap_peek(runtime->cevents) == NULL)? 1 : 0;
}

void ndl_runtime_print(ndl_runtime *runtime) {

    printf("Printing runtime.\n");
    ndl_rhashtable_print(runtime->procs);
    ndl_slabheap_print(runtime->cevents);
}
