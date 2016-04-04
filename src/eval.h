#ifndef NODEL_EVAL_H
#define NODEL_EVAL_H

#include "graph.h"

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
 * When calling a function, you do the following:
 * - Create a new node (self.invoke)
 * - Give it an initial PC (self.invoke.PC = myfunc)
 * - Give it arguments (self.invoke.arg1 = ...)
 * - Give it a return frame (self.invoke.return = self)
 * - Set the local to self.invoke
 * The function now runs.
 * Upon completion, you can access any of its variables, including return values.
 * To return from a function, simply set the local to self.return.
 *
 * These semantics are merely suggestions.
 * The only strict requirements are that self.PC points to the current instruction node.
 */


/* Simulates a single instruction for the frame given by local.
 * If process exits, returns NDL_NULL_REF.
 * If changes frame, returns new local.
 */
ndl_ref ndl_eval(ndl_graph *graph, ndl_ref local);


/* Index and access opcode-implementing functions.
 * Opcodes are each implemented as a separate function, with a similar prototype to ndl_eval.
 * Opcodes can be searched for and enumerated.
 * Lookup is currently O(log(n)), until we switch to hashtables.
 *
 * lookup() gets the evaluation function for the given opcode symbol.
 * size() gets the number of opcodes.
 * index() gets the Nth opcode's symbol.
 */
typedef ndl_ref (*ndl_eval_func)(ndl_graph *graph, ndl_ref local, ndl_ref pc);

ndl_eval_func ndl_eval_lookup(ndl_sym opcode);

int     ndl_eval_size(void);
ndl_sym ndl_eval_index(int index);

#endif /* NODEL_EVAL_H */
