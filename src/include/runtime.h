#ifndef NODEL_RUNTIME_H
#define NODEL_RUNTIME_H

#include "graph.h"

typedef struct ndl_runtime_s ndl_runtime;

/* Create and destroy a runtime.
 * Runtimes encapsulate a node graph and a number of processes.
 *
 * init()  creates a new runtime.
 * kill()  frees all runtime resources and the given runtime.
 *
 * init_using() creates a new runtime using the given graph.
 * graph()      gets the underlying graph of a runtime.
 */
ndl_runtime *ndl_runtime_init(void);
void         ndl_runtime_kill(ndl_runtime *runtime);

ndl_runtime *ndl_runtime_init_using(ndl_graph *graph);
ndl_graph   *ndl_runtime_graph(ndl_runtime *runtime);

/* Manipulate processes.
 *
 * proc_init() makes a new process from the given local block. Returns PID.
 * proc_kill() deletes a process with the given PID.
 */
int ndl_runtime_proc_init (ndl_runtime *runtime, ndl_ref local);
int ndl_runtime_proc_kill (ndl_runtime *runtime, int pid);

/* Simulate processes for one or more steps.
 * Improved debugging support is yet to come.
 * Avoid step_proc(). It's O(n) to find your PID.
 *
 * step_proc() steps the process as pid forward a given number of steps.
 * step() steps all processes forward a given number of steps.
 */
void ndl_runtime_step_proc(ndl_runtime *runtime, int pid, int steps);
void ndl_runtime_step(ndl_runtime *runtime, int steps);

/* Enumerate active processes.
 * PID indices may change every step, this is unordered.
 *
 * proc_count() gets the number of active processes.
 * get_pid() gets the PID of the Nth active process.
 */
int ndl_runtime_proc_count(ndl_runtime *runtime);
int ndl_runtime_get_pid(ndl_runtime *runtime, int tmpindex);

/* For debugging purposes. */
void ndl_runtime_print(ndl_runtime *runtime);

#endif /* NODEL_RUNTIME_H */
