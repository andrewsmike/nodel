#ifndef NODEL_EVAL_H
#define NODEL_EVAL_H

#include "graph.h"

#define NDL_MAX_PROCS 1024

typedef struct ndl_process_s {

    int pid;
    ndl_ref local;
} ndl_process;

typedef struct ndl_runtime_s {

    ndl_graph *nodes;

    int proccount;
    ndl_process procs[NDL_MAX_PROCS];

} ndl_runtime;

ndl_runtime *ndl_runtime_init(void);
void         ndl_runtime_kill(ndl_runtime *runtime);

void         ndl_runtime_step_proc(ndl_runtime *runtime, int pid, int steps);
void         ndl_runtime_step(ndl_runtime *runtime, int steps);

/* Indices are temporary and change on each step() call. *NOT* ordered by PID. */
int          ndl_runtime_proc_count(ndl_runtime *runtime);
int          ndl_runtime_get_pid(ndl_runtime *runtime, int tmpindex);

#endif /* NODEL_EVAL_H */
