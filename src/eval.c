#include "eval.h"
#include "opcodes.h"
#include "hashtable.h"

#include <stdlib.h>

ndl_hashtable *ndl_eval_opcode_table = NULL;

#define ADDOP(table, name, symbol)                                      \
    do {                                                                \
        ndl_eval_func t = &(ndl_opcode_ ## name);                       \
        err = ndl_hashtable_put(table, &NDL_SYM(symbol), &t);           \
        if (err == NULL)                                                \
            return -1;                                                  \
    } while (0)

static int ndl_eval_prep_opcode_table(ndl_hashtable *table) {

    void *err;
    /* Nodes and slots. */
    ADDOP(table, new,  "new     ");
    ADDOP(table, copy, "copy    ");
    ADDOP(table, load, "load    ");
    ADDOP(table, save, "save    ");
    ADDOP(table, drop, "drop    ");
    ADDOP(table, type, "type    ");

    ADDOP(table, count, "count   ");
    ADDOP(table, iload, "iload   ");

    /* Floating point. */
    ADDOP(table, fadd, "fadd    ");
    ADDOP(table, fsub, "fsub    ");
    ADDOP(table, fneg, "fneg    ");
    ADDOP(table, fmul, "fmul    ");
    ADDOP(table, fdiv, "fdiv    ");

    ADDOP(table, fmod,  "fmod    ");
    ADDOP(table, fsqrt, "fsqrt   ");
    ADDOP(table, ftoi,  "ftoi    ");

    /* Integer. */
    ADDOP(table, and, "and     ");
    ADDOP(table, or,  "or      ");
    ADDOP(table, xor, "xor     ");
    ADDOP(table, not, "not     ");
    ADDOP(table, ulshift, "ulshift ");
    ADDOP(table, urshift, "urshift ");

    ADDOP(table, add, "add     ");
    ADDOP(table, sub, "sub     ");
    ADDOP(table, neg, "neg     ");
    ADDOP(table, mul, "mul     ");
    ADDOP(table, div, "div     ");
    ADDOP(table, mod, "mod     ");
    ADDOP(table, lshift, "lshift  ");
    ADDOP(table, rshift, "rshift  ");

    ADDOP(table, itof, "itof    ");
    ADDOP(table, itos, "itos    ");

    /* Symbol. */
    ADDOP(table, stoi, "stoi    ");

    /* Control. */
    ADDOP(table, branch, "branch  ");
    ADDOP(table, push,   "push    ");

    /* Runtime. */
    ADDOP(table, fork,  "fork    ");
    ADDOP(table, exit,  "exit    ");
    ADDOP(table, wait,  "wait    ");
    ADDOP(table, sleep, "sleep   ");

    /* IO. */
    ADDOP(table, excall, "excall  ");

    /* Debugging. */
    ADDOP(table, print,  "print   ");

    return 0;
}

#ifndef NDL_EVAL_OPCODE_TABLE_SIZE

#    define NDL_EVAL_OPCODE_TABLE_SIZE 256

#endif

static inline ndl_hashtable *ndl_eval_get_opcode_table(void) {

    if (ndl_eval_opcode_table == NULL) {

        ndl_hashtable *ret =
            ndl_hashtable_init(sizeof(ndl_sym), sizeof(ndl_eval_func *), NDL_EVAL_OPCODE_TABLE_SIZE);

        if (ret == NULL)
            return NULL;

        int err = ndl_eval_prep_opcode_table(ret);
        if (err != 0)
            ndl_hashtable_kill(ret);

        ndl_eval_opcode_table = ret;
    }

    return ndl_eval_opcode_table;
}

void ndl_eval_opcodes_cleanup(void) {

    if (ndl_eval_opcode_table == NULL)
        return;

    ndl_hashtable_kill(ndl_eval_opcode_table);

    ndl_eval_opcode_table = NULL;

    return;
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

    ndl_eval_func op = ndl_eval_opcode_lookup(opcode.sym);
    if (op == NULL)
        return err; /* Bad instruction. Abort thread. */

    return op(graph, local, pc.ref);
}

ndl_eval_func ndl_eval_opcode_lookup(ndl_sym opcode) {

    ndl_hashtable *ops = ndl_eval_get_opcode_table();
    if (ops == NULL)
        return NULL;

    ndl_eval_func *t = ndl_hashtable_get(ops, &opcode);
    if (t == NULL)
        return NULL;

    return *t;
}

ndl_sym *ndl_eval_opcodes_head(void) {

    ndl_hashtable *ops = ndl_eval_get_opcode_table();
    if (ops == NULL)
        return NULL;

    return (ndl_sym *) ndl_hashtable_keyhead(ops);
}

ndl_sym *ndl_eval_opcodes_next(ndl_sym *last) {

    ndl_hashtable *ops = ndl_eval_get_opcode_table();
    if (ops == NULL)
        return NULL;

    return (ndl_sym *) ndl_hashtable_keynext(ops, (void *) last);
}
