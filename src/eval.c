#include "eval.h"

#include <stdlib.h>

/* Instruction opcode symbol -> invocation function.
 * Sorted by symbol to allow for O(log(n)) lookup.
 */
struct ndl_opcodes_s {
    ndl_eval_func opfunc;
    ndl_sym opcode;
} ndl_opcodes[1] = {
    {NULL, 0}



};


static int ndl_eval_cmp(const void *a, const void *b) {

    const struct ndl_opcodes_s *as = a;
    const struct ndl_opcodes_s *bs = b;

    if (as->opcode < bs->opcode)
        return -1;

    if (as->opcode > bs->opcode)
        return 1;

    return 0;
}

ndl_eval_func ndl_eval_lookup(ndl_sym opcode) {

    struct ndl_opcodes_s key = {
        .opfunc = NULL,
        .opcode = opcode
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
    return ndl_opcodes[index].opcode;
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
