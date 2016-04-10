#include "runtime.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>

struct ndl_process_s {

    int64_t pid;
    ndl_ref local;
};

typedef struct ndl_process_s ndl_process;

#define NDL_MAX_PROCS 1024

struct ndl_runtime_s {

    int free_graph;
    ndl_graph *graph;

    uint64_t proccount;
    int64_t nextpid;
    struct ndl_process_s procs[NDL_MAX_PROCS];
};


ndl_runtime *ndl_runtime_init(ndl_graph *graph) {

    ndl_runtime *ret = malloc(sizeof(ndl_runtime));

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

    ret->proccount = 0;
    ret->nextpid = 0;

    int i;
    for (i = 0; i < NDL_MAX_PROCS; i++)
        ret->procs[i] = (ndl_process) {.pid = -1, .local = NDL_NULL_REF};

    return ret;
}

void ndl_runtime_kill(ndl_runtime *runtime) {

    if (runtime == NULL)
        return;

    if (runtime->graph != NULL && (runtime->free_graph == 1))
        ndl_graph_kill(runtime->graph);

    free(runtime);
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

    ret->proccount = 0;
    ret->nextpid = 0;

    int i;
    for (i = 0; i < NDL_MAX_PROCS; i++)
        ret->procs[i] = (ndl_process) {.pid = -1, .local = NDL_NULL_REF};

    return ret;
}

void ndl_runtime_mkill(ndl_runtime *runtime) {

    if (runtime == NULL)
        return;

    if (runtime->graph != NULL && (runtime->free_graph == 1))
        ndl_graph_kill(runtime->graph);
}

uint64_t ndl_runtime_msize(ndl_graph *graph) {

    return sizeof(ndl_runtime);
}

int64_t ndl_runtime_proc_init(ndl_runtime *runtime, ndl_ref local) {

    if (local == NDL_NULL_REF)
        return -1;

    uint64_t slot = runtime->proccount;

    if (runtime->proccount >= NDL_MAX_PROCS - 1)
        return -1;

    if (ndl_graph_mark(runtime->graph, local) != 0)
        return -1;

    runtime->proccount++;
    int64_t pid = runtime->nextpid++;
    runtime->procs[slot].pid = pid;
    runtime->procs[slot].local = local;

    return pid;
}

static inline int64_t ndl_runtime_proc_index(ndl_runtime *runtime, int64_t pid) {

    uint64_t count = runtime->proccount;

    uint64_t i;
    for (i = 0; i < count; i++) {

        if (runtime->procs[i].pid == pid)
            return (int64_t) i;
    }

    return (int64_t) -1;
}

int ndl_runtime_proc_kill(ndl_runtime *runtime, int64_t pid) {

    int64_t index = ndl_runtime_proc_index(runtime, pid);

    if (index < 0)
        return -1;

    
    uint64_t count = runtime->proccount;

    runtime->procs[index] = runtime->procs[count - 1];
    runtime->procs[count - 1] = (struct ndl_process_s) {.pid = -1, .local = NDL_NULL_REF };
    runtime->proccount--;

    return 0;
}

static void ndl_runtime_tick(ndl_runtime *runtime, uint64_t index) {

    ndl_ref local = runtime->procs[index].local;
    ndl_graph *graph = runtime->graph;

    ndl_eval_result res = ndl_eval(graph, local);

    /* Scan for modified nodes / waits. */
    int exit = 0;

    switch (res.action) {
    case EACTION_CALL:

        if (res.actval.type != EVAL_REF || res.actval.ref == NDL_NULL_REF) {
            exit = 2;
        } else {
            ndl_graph_unmark(graph, runtime->procs[index].local);
            ndl_graph_mark(graph, res.actval.ref);
            runtime->procs[index].local = res.actval.ref;
        }

        break;

    case EACTION_FORK:

        if ((res.actval.type != EVAL_REF) || (res.actval.ref == NDL_NULL_REF))
            exit = 2;
        else
            ndl_runtime_proc_init(runtime, res.actval.ref);

        break;

    case EACTION_NONE:
        break;

    case EACTION_EXIT:
        exit = 1;
        break;

    case EACTION_WAIT:
    case EACTION_SLEEP:
    case EACTION_EXCALL:
        exit = 2;
        break;
    case EACTION_FAIL:
    default:
        exit = 3;
        break;
    }

    if (exit != 0) {
        uint64_t count = runtime->proccount;

        runtime->procs[index] = runtime->procs[count - 1];
        runtime->procs[count - 1] = (struct ndl_process_s) {.pid=-1, .local = NDL_NULL_REF};
        runtime->proccount--;
    }

    if (exit == 2)
        printf("[%3ld] Unsupported feature. Killing.\n", index);
    if (exit == 3)
        printf("[%3ld] Invalid local or bad instruction. Killing.\n", index);
}

void ndl_runtime_step_proc(ndl_runtime *runtime, int64_t pid, uint64_t steps) {

    int64_t index = ndl_runtime_proc_index(runtime, pid);

    if (index < 0)
        return;

    while ((steps-- > 0) && (runtime->procs[index].pid == pid))
        ndl_runtime_tick(runtime, (uint64_t) index);
}

void ndl_runtime_step(ndl_runtime *runtime, uint64_t steps) {

    uint64_t i;
    while (steps-- > 0) {
        if (runtime->proccount == 0)
            return;

        for (i = 0; i < runtime->proccount; i++) {

            int64_t pid = runtime->procs[i].pid;
            ndl_runtime_tick(runtime, i);

            if (runtime->procs[i].pid != pid)
                i--;
        }
    }
}

uint64_t ndl_runtime_proc_count(ndl_runtime *runtime) {
    return runtime->proccount;
}


ndl_graph *ndl_runtime_graph(ndl_runtime *runtime) {
    return runtime->graph;
}

void ndl_runtime_print(ndl_runtime *runtime) {

    printf("Printing runtime.\n");

    uint64_t i;
    for (i = 0; i < runtime->proccount; i++) {

        ndl_value pc = ndl_graph_get(runtime->graph,
                                     runtime->procs[i].local,
                                     NDL_SYM("instpntr"));
        printf("[%3ld] Current instruction@frame: %3d@%03d.\n",
               runtime->procs[i].pid, pc.ref, runtime->procs[i].local);
    }
}
