#ifndef NODEL_OPCODES_H
#define NODEL_OPCODES_H

#include "eval.h"

#define DEFOP(name) \
    ndl_eval_result ndl_opcode_ ## name(ndl_graph *graph, ndl_ref local, ndl_ref pc)

/* Nodes and slots. */
DEFOP(new);
DEFOP(copy);
DEFOP(load);
DEFOP(save);
DEFOP(drop);
DEFOP(type);

DEFOP(count);
DEFOP(iload);

/* Floating point. */
DEFOP(fadd);
DEFOP(fsub);
DEFOP(fneg);
DEFOP(fmul);
DEFOP(fdiv);

DEFOP(fmod);
DEFOP(fsqrt);
DEFOP(ftoi);

/* Integer. */
DEFOP(and);
DEFOP(or);
DEFOP(xor);
DEFOP(not);
DEFOP(ulshift);
DEFOP(urshift);

DEFOP(add);
DEFOP(sub);
DEFOP(neg);
DEFOP(mul);
DEFOP(div);
DEFOP(mod);
DEFOP(lshift);
DEFOP(rshift);

DEFOP(itof);
DEFOP(itos);

/* Symbol. */
DEFOP(stoi);

/* Control. */
DEFOP(branch);
DEFOP(push);

/* Runtime. */
DEFOP(fork);
DEFOP(exit);
DEFOP(wait);
DEFOP(sleep);

/* IO */
DEFOP(excall);

/* Debugging. */
DEFOP(print);

#endif /* NODEL_OPCODES_H */
