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
} ndl_opcodes[46] = {

    /* Nodes and slots. */
    ADDOP(new,  "new     "),
    ADDOP(copy, "copy    "),
    ADDOP(load, "load    "),
    ADDOP(save, "save    "),
    ADDOP(drop, "drop    "),

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

    /* Cotrol. */
    ADDOP(branch, "branch  "),
    ADDOP(push,   "push    "),
};

static int ndl_eval_cmp(const void *a, const void *b) {

    const struct ndl_opcodes_s *as = a;
    const struct ndl_opcodes_s *bs = b;

    int64_t ap = NDL_SYM(as->opcode);
    int64_t bp = NDL_SYM(bs->opcode);

    if (ap < bp)
        return -1;

    if (ap > bp)
        return 1;

    return 0;
}

ndl_eval_func ndl_eval_lookup(ndl_sym opcode) {

    struct ndl_opcodes_s key = {
        .opfunc = NULL,
        .opcode = (char*) &opcode,
    };

    struct ndl_opcodes_s *t =
        bsearch(&key, ndl_opcodes,
                ndl_eval_size(), sizeof(struct ndl_opcodes_s),
                &ndl_eval_cmp);

    if (t == NULL)
        return NULL;
    else
        return t->opfunc;
}

int ndl_eval_size(void) {
    return 0;
}

ndl_sym ndl_eval_index(int index) {
    return NDL_SYM(ndl_opcodes[index].opcode);
}

ndl_ref ndl_eval(ndl_graph *graph, ndl_ref local) {

    ndl_value pc = ndl_graph_get(graph, local, NDL_SYM("instpntr"));
    if (pc.type != EVAL_REF || pc.ref == NDL_NULL_REF)
        return NDL_NULL_REF;  /* Bad local. Abort thread. */

    ndl_value opcode = ndl_graph_get(graph, pc.ref, NDL_SYM("opcode  "));
    if (opcode.type != EVAL_SYM)
        return NDL_NULL_REF; /* Bad instruction. Abort thread. */

    ndl_eval_func op = ndl_eval_lookup(opcode.sym);
    if (op == NULL)
        return NDL_NULL_REF; /* Bad instruction. Abort thread. */

    return op(graph, local, pc.ref);
}
