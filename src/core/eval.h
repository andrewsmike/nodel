#ifndef NODEL_EVAL_H
#define NODEL_EVAL_H

#include "graph.h"
#include "excall.h"

/* Instructions are represented as nodes in the graph.
 * Each instruction has a number of variables, based on its specific needs.
 * Each instruction also points to the next instruction (or multiple instructions.)
 * All instruction nodes have an 'opcode' key, which is a symbol.
 * Invalid opcodes cause an exit.
 *
 * Each process is associated with a 'local' node.
 * This represents the current stack frame.
 * It has a PC, possibly a return frame, and data.
 * All operations take place on this local node.
 * Each operation, the PC advances, the local block may be replaced.
 *
 * To call a function, you can do the following:
 * - Create a new node (self.invoke)
 * - Give it an initial PC (self.invoke.instpntr = myfunc)
 * - Give it arguments (self.invoke.arg1 = ...)
 * - Give it a return frame (self.invoke.return = self)
 * - Set the local to self.invoke
 * The function now runs.
 * Upon completion, you can access any of its variables, including return values.
 * To return from a function, simply set the local to self.return.
 *
 * These semantics are merely a suggestion.
 * The only strict requirements are that self.instpntr points to the
 * current instruction node. To launch a process, just create its local frame,
 * set its arguments, set the self.instpntr, and hand it to runtime.
 */

/* Evaluation keeps track of touched nodes, next frame, fork calls, and more.
 * Returns all modified nodes (including the local frame) and a runtime-oriented
 * action. res.action has a number of types. It uses res.actval as its argument.
 *
 * mod_count: Number of nodes touched by the opcode. Includes local.
 * mod: Up to three addresses touched by the opcode. Trash when not in use.
 *     May include repeats. Includes local block.
 *
 * action: Action to take using actval.
 * actval: Value of action to take. Could be timeout, reference for wait,
 *     local for for a forked process, etc. Trash when not in use.
 */
typedef struct ndl_eval_result_s {

    uint8_t mod_count;
    ndl_ref mod[3];

    enum ndl_action_e {

        EACTION_NONE,  /* Continue executing normally. */

        EACTION_CALL,  /* Set local=actval.ref. */

        EACTION_FORK,  /* Create new proc with local=actval.ref. */
        EACTION_EXIT,  /* Exit gracefully. */
        EACTION_FAIL,  /* Exit... roughly. */

        EACTION_WAIT,  /* Sleep until actval.ref is modified. */
        EACTION_SLEEP, /* Sleep for actval.num milliseconds. */

        EACTION_EXCALL, /* External call. actval = last instpntr. Special handling. */

        EACTION_SIZE

    } action;

    ndl_value actval;

} ndl_eval_result;

/* Simulates a single instruction for the frame given by local.
 * If process exits, returns NDL_NULL_REF.
 * If changes frame, returns new local.
 */
ndl_eval_result ndl_eval(ndl_graph *graph, ndl_ref local);


/* Opcode lookup table.
 * Opcodes are each implemented as a separate function, with a similar prototype to ndl_eval.
 * Opcodes can be queried by key and their keys enumerated.
 * Runs on a symbol->ndl_eval_func resizable hashtable backend, avoids too much iteration.
 * Iteration methods return NULL on end-of-list.
 * Also creates and destroys an excall table with the opcode table.
 * Excall table can be fetched and manipulated.
 *
 * opcode_lookup() gets the evaluation function for the given opcode symbol.
 *
 * opcodes_head() gets the iterator to the first opcode key in the hashtable.
 * opcodes_next() gets the iterator to the next opcode key in the hashtable.
 * opcodes_get() gets the symbol for opcode at the iterator.
 *
 * opcodes_ref() adds a reference to the opcode system. May generate table.
 * opcodes_deref() removes a reference to the opcode table. If refcount == 0, frees.
 *
 * excall() gets the excall table.
 *     TODO: Move opcode and eval table to separate object?
 */
typedef ndl_eval_result (*ndl_eval_func)(ndl_graph *graph, ndl_ref local, ndl_ref pc);

ndl_eval_func ndl_eval_opcode_lookup(ndl_sym opcode);

void *ndl_eval_opcodes_head(void);
void *ndl_eval_opcodes_next(void *prev);
ndl_sym ndl_eval_opcodes_get(void *curr);

void ndl_eval_opcodes_ref(void);
void ndl_eval_opcodes_deref(void);

ndl_excall *ndl_eval_excall(void);

#endif /* NODEL_EVAL_H */
