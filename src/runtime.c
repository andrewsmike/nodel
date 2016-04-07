#include "runtime.h"
#include "eval.h"

#include <stdlib.h>

struct ndl_process_s {

    int pid;
    ndl_ref local;
};

typedef struct ndl_process_s ndl_process;


#define NDL_MAX_PROCS 1024

struct ndl_runtime_s {

    ndl_graph *graph;

    int proccount;
    int nextpid;
    struct ndl_process_s procs[NDL_MAX_PROCS];
};

ndl_runtime *ndl_runtime_init(void) {

    ndl_runtime *ret = malloc(sizeof(ndl_runtime));

    if (ret == NULL)
        return NULL;

    ret->graph = ndl_graph_init();

    if (ret->graph == NULL) {
        free(ret);
        return NULL;
    }

    ret->proccount = 0;
    ret->nextpid = 0;

    int i;
    for (i = 0; i < NDL_MAX_PROCS; i++)
        ret->procs[i] = (ndl_process) {.pid = -1, .local = NDL_NULL_REF};

    return ret;
}

ndl_runtime *ndl_runtime_sinit(ndl_graph *graph) {

    ndl_runtime *ret = malloc(sizeof(ndl_runtime));

    if (ret == NULL)
        return NULL;

    ret->graph = graph;

    if (ret->graph == NULL) {
        free(ret);
        return NULL;
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

    if (runtime->graph != NULL)
        ndl_graph_kill(runtime->graph);

    free(runtime);
}

ndl_graph *ndl_runtime_graph(ndl_runtime *runtime) {
    return runtime->graph;
}

int ndl_runtime_proc_init(ndl_runtime *runtime, ndl_ref local) {

    if (local == NDL_NULL_REF)
        return -1;

    int slot = runtime->proccount;

    if (runtime->proccount >= NDL_MAX_PROCS - 1)
        return -1;

    runtime->proccount++;
    int pid = runtime->nextpid++;
    runtime->procs[slot].pid = pid;
    runtime->procs[slot].local = local;

    return 0;
}

int ndl_runtime_proc_kill(ndl_runtime *runtime, int pid) {

    int count = runtime->proccount;

    int i;
    for (i = 0; i < count; i++) {

        if (runtime->procs[i].pid == pid) {

            runtime->procs[i] = runtime->procs[count - 1];
            runtime->procs[count-1] = (struct ndl_process_s) {.pid = -1, .local = NDL_NULL_REF };
            runtime->proccount--;
            return 0;
        }
    }

    return -1;
}

static void ndl_runtime_tick(ndl_runtime *runtime, int index) {

    ndl_ref local = runtime->procs[index].local;
    ndl_graph *graph = runtime->graph;

    ndl_ref next = ndl_eval(graph, local);

    if (next == local)
        return;

    if (next != NDL_NULL_REF) {
        runtime->procs[index].local = next;
        return;
    }

    int count = runtime->proccount;

    runtime->procs[index] = runtime->procs[count - 1];
    runtime->procs[count - 1] = (struct ndl_process_s) {.pid=-1, .local = NDL_NULL_REF};
    runtime->proccount--;
}

void ndl_runtime_step_proc(ndl_runtime *runtime, int pid, int steps) {

    int i;
    for (i = 0; i < runtime->proccount; i++)
        if (runtime->procs[i].pid == pid)
            while ((steps-- > 0) && (runtime->procs[i].pid == pid))
                ndl_runtime_tick(runtime, i);
}

void ndl_runtime_step(ndl_runtime *runtime, int steps) {

    int i;
    while (steps-- > 0) {
        for (i = 0; i < runtime->proccount; i++) {

            int pid = runtime->procs[i].pid;
            ndl_runtime_tick(runtime, i);

            if (runtime->procs[i].pid != pid)
                i--;
        }
    }
}

int ndl_runtime_proc_count(ndl_runtime *runtime) {
    return runtime->proccount;
}

int ndl_runtime_get_pid(ndl_runtime *runtime, int tmpindex) {
    return runtime->procs[tmpindex].pid;
}

#include <stdio.h>

void ndl_runtime_print(ndl_runtime *runtime) {

    printf("Printing runtime.\n");

    int i;
    for (i = 0; i < runtime->proccount; i++) {

        ndl_value pc = ndl_graph_get(runtime->graph,
                                     runtime->procs[i].local,
                                     NDL_SYM("instpntr"));
        printf("[%3d] Current instruction@frame: %3d@%03d.\n",
               runtime->procs[i].pid, pc.ref, runtime->procs[i].local);
    }
}
