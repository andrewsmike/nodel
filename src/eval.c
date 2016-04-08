#include "eval.h"
#include "opcodes.h"

#include <stdlib.h>

#define ADDOP(name, symbol) {&(ndl_opcode_ ## name), symbol}

/* Instruction opcode symbol -> invocation function.
 * Sorted by symbol to allow for O(log(n)) lookup.
 */
struct ndl_opcodes_s {
    ndl_eval_func opfunc;
    const char *opcode;
} ndl_opcodes[40] = {

    /* Nodes and slots. */
    ADDOP(new,  "new     "),
    ADDOP(copy, "copy    "),
    ADDOP(load, "load    "),
    ADDOP(save, "save    "),
    ADDOP(drop, "drop    "),
    ADDOP(type, "type    "),

    ADDOP(count, "count   "),
    ADDOP(iload, "iload   "),

    /* Floating point. */
    ADDOP(fadd, "fadd    "),
    ADDOP(fsub, "fsub    "),
    ADDOP(fneg, "fneg    "),
    ADDOP(fmul, "fmul    "),
    ADDOP(fdiv, "fdiv    "),

    ADDOP(fmod,  "fmod    "),
    ADDOP(fsqrt, "fsqrt   "),
    ADDOP(ftoi,  "ftoi    "),

    /* Integer. */
    ADDOP(and, "and     "),
    ADDOP(or,  "or      "),
    ADDOP(xor, "xor     "),
    ADDOP(not, "not     "),
    ADDOP(ulshift, "ulshift "),
    ADDOP(urshift, "urshift "),

    ADDOP(add, "add     "),
    ADDOP(sub, "sub     "),
    ADDOP(neg, "neg     "),
    ADDOP(mul, "mul     "),
    ADDOP(div, "div     "),
    ADDOP(mod, "mod     "),
    ADDOP(lshift, "lshift  "),
    ADDOP(rshift, "rshift  "),

    ADDOP(itof, "itof    "),
    ADDOP(itos, "itos    "),

    /* Symbol. */
    ADDOP(stoi, "stoi    "),

    /* Control. */
    ADDOP(branch, "branch  "),
    ADDOP(push,   "push    "),

    /* Runtime. */
    ADDOP(fork,  "fork    "),
    ADDOP(exit,  "exit    "),
    ADDOP(wait,  "wait    "),
    ADDOP(sleep, "sleep   "),

    /* Temporary / debugging. */
    ADDOP(print,  "print   "),
};

ndl_eval_func ndl_eval_lookup(ndl_sym opcode) {

    int count = ndl_eval_size();

    int i;
    for (i = 0; i < count; i++)
        if (NDL_SYM(ndl_opcodes[i].opcode) == opcode)
            return ndl_opcodes[i].opfunc;

    return NULL;
}

int ndl_eval_size(void) {
    return 47;
}

ndl_sym ndl_eval_index(int index) {
    return NDL_SYM(ndl_opcodes[index].opcode);
}

ndl_eval_result ndl_eval(ndl_graph *graph, ndl_ref local) {

    ndl_eval_result err;
    err.mod_count = 0;
    err.action = EACTION_FAIL;

    ndl_value pc = ndl_graph_get(graph, local, NDL_SYM("instpntr"));
    if (pc.type != EVAL_REF || pc.ref == NDL_NULL_REF)
        return err;  /* Bad local. Abort thread. */

    ndl_value opcode = ndl_graph_get(graph, pc.ref, NDL_SYM("opcode  "));
    if (opcode.type != EVAL_SYM)
        return err; /* Bad instruction. Abort thread. */

    ndl_eval_func op = ndl_eval_lookup(opcode.sym);
    if (op == NULL)
        return err; /* Bad instruction. Abort thread. */

    return op(graph, local, pc.ref);
}
