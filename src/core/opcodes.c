#include "opcodes.h"
#include "eval.h"

#include <math.h>
#include <stdio.h>

/* This file generates all of the opcode functions used by nodel.
 * For brevity, a wide variety of macros are provided.
 * May move to a separate description file and generator later.
 *
 * Assertion macros abort the thread if their condition is not met.
 * Load and store macros do as advertised. They may use assertions.
 * BEGINOP(), ADVANCE, and LOADSYM[a[b[c]]] are convenience macros.
 */

#define BEGINOP(name) \
    ndl_eval_result ndl_opcode_ ## name(ndl_graph *graph, ndl_ref local, ndl_ref pc)

#define INITRES                \
    ndl_eval_result res;       \
    res.mod_count = 0;         \
    res.action = EACTION_NONE

#define FAIL                       \
    do {                           \
        res.action = EACTION_FAIL; \
        return res;                \
    } while (0)

#define ASSERTTYPE(value, etype) \
    do {                         \
        if (value.type != etype) \
            FAIL;                \
    } while (0)

#define ASSERTNOTNONE(value)         \
    do {                             \
        if (value.type == EVAL_NONE) \
            FAIL;                    \
    } while (0)

#define ASSERTREF(value)               \
    ASSERTTYPE(value, EVAL_REF);       \
    do {                               \
        if (value.ref == NDL_NULL_REF) \
            FAIL;                      \
    } while (0)

#define LOAD(node, name, sym, type)             \
    ndl_value name;                             \
    do {                                        \
        name = ndl_graph_get(graph, node, sym); \
        ASSERTTYPE(name, type);                 \
    } while (0)

#define LOADREF(node, name, sym)                \
    ndl_value name;                             \
    do {                                        \
        name = ndl_graph_get(graph, node, sym); \
        ASSERTREF(name);                        \
    } while (0)

#define LOADVAL(node, node2, name, esym, etype)         \
    ndl_value name;                                     \
    do {                                                \
        name = ndl_graph_get(graph, node, esym);        \
        if (name.type == EVAL_SYM) {                    \
            LOAD(node2, name ## 2, name.sym, etype);    \
            name = name ## 2;                           \
        } else if (name.type != etype) {                \
            FAIL;                                       \
        }                                               \
    } while (0)

#define NTLOADVAL(node, node2, name, esym)      \
    ndl_value name;                             \
    do {                                        \
        name = ndl_graph_get(graph, node, esym);\
        if (name.type == EVAL_SYM) {            \
            NTLOAD(node2, name ## 2, name.sym); \
            ASSERTNOTNONE(name ## 2);           \
            name = name ## 2;                   \
        } else if (name.type == EVAL_NONE) {    \
            FAIL;                               \
        }                                       \
    } while (0)

#define NTLOAD(node, name, sym)                 \
    ndl_value name;                             \
    do {                                        \
        name = ndl_graph_get(graph, node, sym); \
        ASSERTNOTNONE(name);                    \
    } while (0)

#define STORE(node, value, sym)                           \
    do {                                                  \
        res.mod[res.mod_count++] = node;                  \
        int err = ndl_graph_set(graph, node, sym, value); \
        if (err != 0)                                     \
            FAIL;                                         \
    } while (0)

#define DROP(node, sym)                            \
    do {                                           \
        res.mod[res.mod_count++] = node;           \
        int err = ndl_graph_del(graph, node, sym); \
        if (err != 0)                              \
            FAIL;                                  \
    } while (0)

#define ADVANCE                             \
    do {                                    \
        LOADREF(pc, next, DS("next    "));  \
        STORE(local, next, DS("instpntr")); \
        return res;                         \
    } while (0)

#define DS(name) NDL_SYM(name)

#define LOADSYMA LOAD(pc, syma, DS("syma    "), EVAL_SYM)
#define LOADSYMAB LOADSYMA; LOAD(pc, symb, DS("symb    "), EVAL_SYM)
#define LOADSYMABC LOADSYMAB; LOAD(pc, symc, DS("symc    "), EVAL_SYM)

#define NEWLINKED(node, name, sym)                     \
    ndl_value name;                                    \
    do {                                               \
        name.type = EVAL_REF;                          \
        name.ref = ndl_graph_salloc(graph, node, sym); \
        res.mod[res.mod_count++] = node;               \
        res.mod[res.mod_count++] = name.ref;           \
        ASSERTREF(name);                               \
    } while (0)

BEGINOP(new) {
    INITRES;
    LOADSYMA;

    NEWLINKED(local, new, syma.sym);

    ADVANCE;
}

BEGINOP(copy) {
    INITRES;

    NTLOADVAL(pc, local, val, DS("syma    "));
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);

    STORE(local, val, symb.sym);

    ADVANCE;
}

BEGINOP(load) {
    INITRES;

    LOADVAL(pc, local, sec, DS("syma    "), EVAL_REF);
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);
    LOAD(pc, symc, DS("symc    "), EVAL_SYM);

    NTLOAD(sec.ref, val, symb.sym);

    STORE(local, val, symc.sym);

    ADVANCE;
}

BEGINOP(save) {
    INITRES;

    NTLOADVAL(pc, local, val, DS("syma    "));
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);
    LOADVAL(pc, local, sec, DS("symc    "), EVAL_REF);

    STORE(sec.ref, val, symb.sym);

    ADVANCE;
}

BEGINOP(drop) {
    INITRES;
    LOADSYMAB;

    LOADREF(local, sec, syma.sym);
    DROP(sec.ref, symb.sym);

    ADVANCE;
}

BEGINOP(type) {
    INITRES;
    LOADSYMABC;

    LOADREF(local, sec, syma.sym);
    ndl_value val = ndl_graph_get(graph, sec.ref, symb.sym);

    char *type;
    switch (val.type) {
    case EVAL_NONE:  type = "none    "; break;
    case EVAL_REF:   type = "ref     "; break;
    case EVAL_SYM:   type = "sym     "; break;
    case EVAL_INT:   type = "int     "; break;
    case EVAL_FLOAT: type = "float   "; break;
    default:         FAIL;              break;
    }

    ndl_sym typesym = NDL_SYM(type);

    STORE(local, NDL_VALUE(EVAL_SYM, sym=typesym), symc.sym);

    ADVANCE;
}

BEGINOP(count) {
    INITRES;
    LOADSYMAB;

    LOADREF(local, sec, syma.sym);
    int64_t size = ndl_graph_size(graph, sec.ref);
    STORE(local, NDL_VALUE(EVAL_INT, num=size), symb.sym);

    ADVANCE;
}

BEGINOP(iload) {
    INITRES;
    LOADSYMABC;

    LOADREF(local, sec, syma.sym);
    LOAD(local, i, symb.sym, EVAL_INT);
    ndl_sym key = ndl_graph_index(graph, sec.ref, i.num);
    STORE(local, NDL_VALUE(EVAL_SYM, sym=key), symc.sym);

    ADVANCE;
}

#define ONEARGFPOP(name, expr)                                      \
    BEGINOP(name) {                                                 \
        INITRES;                                                    \
        LOADVAL(pc, local, a, DS("syma    "), EVAL_FLOAT);          \
        LOAD(pc, symb, DS("symb    "), EVAL_SYM);                   \
        STORE(local, NDL_VALUE(EVAL_FLOAT, real=(expr)), symb.sym); \
        ADVANCE;                                                    \
    }

#define TWOARGFPOP(name, expr)                                      \
    BEGINOP(name) {                                                 \
        INITRES;                                                    \
        LOADVAL(pc, local, a, DS("syma    "), EVAL_FLOAT);          \
        LOADVAL(pc, local, b, DS("symb    "), EVAL_FLOAT);          \
        LOAD(pc, symc, DS("symc    "), EVAL_SYM);                   \
        STORE(local, NDL_VALUE(EVAL_FLOAT, real=(expr)), symc.sym); \
        ADVANCE;                                                    \
    }

TWOARGFPOP(fadd, a.real + b.real)
TWOARGFPOP(fsub, a.real - b.real)
ONEARGFPOP(fneg, - a.real)
TWOARGFPOP(fmul, a.real * b.real)
TWOARGFPOP(fdiv, a.real / b.real)
TWOARGFPOP(fmod, fmod(a.real, b.real))
ONEARGFPOP(fsqrt, sqrt(a.real))

BEGINOP(ftoi) {
    INITRES;
    LOADVAL(pc, local, a, DS("syma    "), EVAL_FLOAT);
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);

    STORE(local, NDL_VALUE(EVAL_INT, num=(int)a.real), symb.sym);

    ADVANCE;
}

#define ONEARGINTOP(name, expr)                                  \
    BEGINOP(name) {                                              \
        INITRES;                                                 \
        LOADVAL(pc, local, a, DS("syma    "), EVAL_INT);         \
        LOAD(pc, symb, DS("symb    "), EVAL_SYM);                \
        STORE(local, NDL_VALUE(EVAL_INT, num=(expr)), symb.sym); \
        ADVANCE;                                                 \
    }

#define TWOARGINTOP(name, expr)                                  \
    BEGINOP(name) {                                              \
        INITRES;                                                 \
        LOADVAL(pc, local, a, DS("syma    "), EVAL_INT);         \
        LOADVAL(pc, local, b, DS("symb    "), EVAL_INT);         \
        LOAD(pc, symc, DS("symc    "), EVAL_SYM);                \
        STORE(local, NDL_VALUE(EVAL_INT, num=(expr)), symc.sym); \
        ADVANCE;                                                 \
    }

TWOARGINTOP(and, a.num & b.num)
TWOARGINTOP(or,  a.num | b.num)
TWOARGINTOP(xor, a.num ^ b.num)
ONEARGINTOP(not, ~a.num)
TWOARGINTOP(lshift, a.num << b.num)
TWOARGINTOP(rshift, a.num >> b.num)
TWOARGINTOP(ulshift, (int64_t) (((uint64_t) a.num) << b.num))
TWOARGINTOP(urshift, (int64_t) (((uint64_t) a.num) >> b.num))

TWOARGINTOP(add, a.num + b.num)
TWOARGINTOP(sub, a.num - b.num)
ONEARGINTOP(neg, - a.num)
TWOARGINTOP(mul, a.num * b.num)
TWOARGINTOP(div, a.num / b.num)
TWOARGINTOP(mod, a.num % b.num)

BEGINOP(itof) {
    INITRES;
    LOADVAL(pc, local, a, DS("syma    "), EVAL_INT);
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);

    STORE(local, NDL_VALUE(EVAL_FLOAT, real=(double)a.num), symb.sym);

    ADVANCE;
}

BEGINOP(itos) {
    INITRES;
    LOADVAL(pc, local, a, DS("syma    "), EVAL_INT);
    LOAD(pc, symb, DS("symb    "), EVAL_SYM);

    STORE(local, NDL_VALUE(EVAL_SYM, sym=(ndl_sym) a.num), symb.sym);

    ADVANCE;
}

BEGINOP(stoi) {
    INITRES;
    LOADSYMAB;

    LOAD(local, a, syma.sym, EVAL_SYM);
    STORE(local, NDL_VALUE(EVAL_INT, num=(int64_t) a.sym), symb.sym);

    ADVANCE;
}

BEGINOP(branch) {
    INITRES;

    NTLOADVAL(pc, local, a, DS("syma    "));
    LOADVAL(pc, local, b, DS("symb    "), a.type);

    int cmp;

    switch (a.type) {
    case EVAL_INT:
    case EVAL_SYM:
    case EVAL_REF:
        if (a.num < b.num) cmp = -1;
        else if (a.num == b.num) cmp = 0;
        else cmp = 1;
        break;
    case EVAL_FLOAT:
        if (a.real < b.real) cmp = -1;
        else if (a.real == b.real) cmp = 0;
        else cmp = 1;
        break;
    case EVAL_NONE:
        cmp = 0;
        break;
    default:
        FAIL;
    }

    ndl_sym branch;

    if (cmp == -1) branch = DS("lt      ");
    if (cmp ==  0) branch = DS("eq      ");
    if (cmp ==  1) branch = DS("gt      ");

    ndl_value next = ndl_graph_get(graph, pc, branch);
    if (next.type == EVAL_NONE) {
        next = ndl_graph_get(graph, pc, DS("next    "));
    }
    ASSERTNOTNONE(next);

    STORE(local, next, DS("instpntr"));

    return res;
}

BEGINOP(push) {
    INITRES;
    LOADSYMA;

    LOADREF(pc, next, DS("next    "));
    STORE(local, next, DS("instrpntr"));

    LOADREF(local, invoke, syma.sym);

    res.action = EACTION_CALL;
    res.actval = invoke;

    return res;
}

BEGINOP(fork) {
    INITRES;
    LOADSYMA;

    LOADREF(local, fork, syma.sym);

    res.action = EACTION_FORK;
    res.actval = fork;

    ADVANCE;
}

BEGINOP(exit) {
    INITRES;

    res.action = EACTION_EXIT;

    return res;
}

BEGINOP(sleep) {
    INITRES;

    LOADVAL(pc, local, val, DS("syma    "), EVAL_INT);

    res.action = EACTION_SLEEP;
    res.actval = val;

    ADVANCE;
}

BEGINOP(wait) {
    INITRES;

    LOADVAL(pc, local, val, DS("syma    "), EVAL_REF);
    ASSERTREF(val);

    res.action = EACTION_WAIT;
    res.actval = val;

    ADVANCE;
}

BEGINOP(excall) {
    INITRES;

    res.action = EACTION_EXCALL;
    res.actval.type = EVAL_REF;
    res.actval.ref = pc;

    ADVANCE;
}

BEGINOP(print) {
    INITRES;

    NTLOADVAL(pc, local, val, DS("syma    "));

    char buff[16];
    buff[15] = '\0';

    ndl_value_to_string(val, 15, buff);

    printf("[%03ld@%03ld]: %s.\n", pc, local, buff);

    ADVANCE;
}
