/** nodel/src/test.c: Test implemented functionality. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "node.h"
#include "graph.h"
#include "runtime.h"
#include "slab.h"
#include "hashtable.h"
#include "rehashtable.h"
#include "eval.h"
#include "asm.h"

static int testprettyprint(void) {

    printf("Testing value pretty printing.\n");

    ndl_value values[] = {
        NDL_VALUE(EVAL_NONE, ref=27),
        NDL_VALUE(EVAL_REF, ref=NDL_NULL_REF),
        NDL_VALUE(EVAL_REF, ref=0x0001),
        NDL_VALUE(EVAL_REF, ref=0xABC4),
        NDL_VALUE(EVAL_REF, ref=0xABCDEF),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("hello   ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("\0hello ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("\0\0\0\0\0\0\0\0")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("        ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("next    ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("    last")),
        NDL_VALUE(EVAL_INT, num=0),
        NDL_VALUE(EVAL_INT, num=-1),
        NDL_VALUE(EVAL_INT, num=1),
        NDL_VALUE(EVAL_INT, num=10000),
        NDL_VALUE(EVAL_INT, num=10000009),
        NDL_VALUE(EVAL_INT, num=-10000000009),
        NDL_VALUE(EVAL_FLOAT, real=-100000000.0),
        NDL_VALUE(EVAL_FLOAT, real=100000000.0),
        NDL_VALUE(EVAL_FLOAT, real=1000.03),
        NDL_VALUE(EVAL_FLOAT, real=0.00000000004),
        NDL_VALUE(EVAL_FLOAT, real=-0.00000000004),
        NDL_VALUE(EVAL_FLOAT, real=-0.00434),
        NDL_VALUE(EVAL_FLOAT, real=1.3287),
        NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF)
    };

    char buff[16];
    buff[15] = '\0';

    int i;
    for (i = 0; values[i].type != EVAL_NONE || values[i].ref != NDL_NULL_REF; i++) {
        ndl_value_to_string(values[i], 15, buff);
        printf("%2ith value: %s.\n", i, buff);
    }

    return 0;
}

static ndl_ref testgraphalloc(ndl_graph *graph) {

    ndl_ref ret = ndl_graph_alloc(graph);

    if (ret == NDL_NULL_REF) {
        fprintf(stderr, "Failed to allocate graph node.\n");
        exit(EXIT_FAILURE);
    }

    return ret;
}

static void testgraphaddedge(ndl_graph *graph, ndl_ref a, ndl_ref b, const char *name) {

    int err = ndl_graph_set(graph, a, NDL_SYM(name), NDL_VALUE(EVAL_REF, ref=b));

    if (err != 0) {
        fprintf(stderr, "Failed to add edge\n");
        exit(EXIT_FAILURE);
    }
}

static void testgraphdeledge(ndl_graph *graph, ndl_ref a, const char *name) {

    int err = ndl_graph_del(graph, a, NDL_SYM(name));

    if (err != 0) {
        fprintf(stderr, "Failed to delete edge\n");
        exit(EXIT_FAILURE);
    }
}

static int testgraph(void) {

    printf("Testing graph.\n");

    ndl_graph *graph = ndl_graph_init();

    if (graph == NULL) {
        fprintf(stderr, "Failed to allocate graph.\n");
        return -1;
    }

    printf("Allocating nodes A, B, and C.\n");
    ndl_ref a = testgraphalloc(graph);
    ndl_ref b = testgraphalloc(graph);
    ndl_ref c = testgraphalloc(graph);

    ndl_graph_print(graph);

    printf("Adding edges a.b = b, b.c = c, c.a = a.\n");
    testgraphaddedge(graph, a, b, "b       ");
    testgraphaddedge(graph, b, c, "c       ");
    testgraphaddedge(graph, c, a, "a       ");

    ndl_graph_print(graph);

    printf("Adding edges b.last = c, b.first = c.\n");
    testgraphaddedge(graph, b, c, "last    ");
    testgraphaddedge(graph, b, c, "first   ");

    ndl_graph_print(graph);

    printf("Removing edges b.* = c.\n");
    testgraphdeledge(graph, b, "c       ");
    testgraphdeledge(graph, b, "last    ");
    testgraphdeledge(graph, b, "first   ");
    ndl_graph_print(graph);

    ndl_graph_unmark(graph, a);
    ndl_graph_salloc(graph, b, NDL_SYM("back    "));

    ndl_graph_print(graph);

    ndl_graph_kill(graph);

    graph = ndl_graph_init();

    if (graph == NULL) {
        fprintf(stderr, "Failed to allocate graph.\n");
        return -1;
    }

    a = testgraphalloc(graph);
    b = testgraphalloc(graph);
    c = testgraphalloc(graph);
    testgraphaddedge(graph, a, b, "next    ");

    ndl_ref d = ndl_graph_salloc(graph, b, NDL_SYM("next    "));
    ndl_ref e = ndl_graph_salloc(graph, d, NDL_SYM("next    "));

    ndl_ref f = ndl_graph_salloc(graph, e, NDL_SYM("next    "));
    ndl_ref g = ndl_graph_salloc(graph, f, NDL_SYM("next    "));
    ndl_ref h = ndl_graph_salloc(graph, g, NDL_SYM("next    "));
    testgraphaddedge(graph, h, f, "next    ");
    testgraphaddedge(graph, h, a, "root    ");

    printf("Testing GC.\n");
    ndl_graph_print(graph);

    ndl_graph_clean(graph);

    printf("Post GC.\n");
    ndl_graph_print(graph);

    testgraphdeledge(graph, e, "next    ");

    printf("Pre GC.\n");
    ndl_graph_print(graph);

    ndl_graph_clean(graph);

    printf("Post GC.\n");
    ndl_graph_print(graph);

    ndl_graph_unmark(graph, a);
    ndl_graph_clean(graph);
    printf("Unmarking a as root node.\n");
    ndl_graph_print(graph);

    ndl_graph_unmark(graph, b);
    ndl_graph_mark(graph, e);
    ndl_graph_clean(graph);
    printf("Unmarking b as root node and marking e.\n");
    ndl_graph_print(graph);

    ndl_graph_kill(graph);

    return 0;
}

#define SET(node, key, type, val) \
    ndl_graph_set(graph, node, NDL_SYM(key), NDL_VALUE(type, val))

static int testruntimeadd(void) {

    printf("Beginning runtime addition tests.\n");

    ndl_runtime *runtime = ndl_runtime_init(NULL);

    if (runtime == NULL) {
        fprintf(stderr, "Failed to allocate runtime.\n");
        exit(EXIT_FAILURE);
    }

    ndl_graph *graph = ndl_runtime_graph(runtime);

    ndl_ref i0 = testgraphalloc(graph);
    ndl_ref i1 = ndl_graph_salloc(graph, i0, NDL_SYM("next    "));
    ndl_ref i2 = ndl_graph_salloc(graph, i1, NDL_SYM("next    "));
    ndl_ref i3 = ndl_graph_salloc(graph, i2, NDL_SYM("next    "));
    ndl_ref i4 = ndl_graph_salloc(graph, i3, NDL_SYM("next    "));

    SET(i0, "opcode  ", EVAL_SYM, sym=NDL_SYM("load    "));
    SET(i0, "syma    ", EVAL_SYM, sym=NDL_SYM("instpntr"));
    SET(i0, "symb    ", EVAL_SYM, sym=NDL_SYM("const   "));
    SET(i0, "symc    ", EVAL_SYM, sym=NDL_SYM("a       "));
    SET(i0, "const   ", EVAL_INT, num=2);

    SET(i1, "opcode  ", EVAL_SYM, sym=NDL_SYM("load    "));
    SET(i1, "syma    ", EVAL_SYM, sym=NDL_SYM("instpntr"));
    SET(i1, "symb    ", EVAL_SYM, sym=NDL_SYM("const   "));
    SET(i1, "symc    ", EVAL_SYM, sym=NDL_SYM("b       "));
    SET(i1, "const   ", EVAL_INT, num=2);

    SET(i2, "opcode  ", EVAL_SYM, sym=NDL_SYM("add     "));
    SET(i2, "syma    ", EVAL_SYM, sym=NDL_SYM("a       "));
    SET(i2, "symb    ", EVAL_SYM, sym=NDL_SYM("b       "));
    SET(i2, "symc    ", EVAL_SYM, sym=NDL_SYM("c       "));

    SET(i3, "opcode  ", EVAL_SYM, sym=NDL_SYM("print   "));
    SET(i3, "syma    ", EVAL_SYM, sym=NDL_SYM("c       "));

    SET(i4, "opcode  ", EVAL_SYM, sym=NDL_SYM("exit    "));

    ndl_ref local = testgraphalloc(graph);
    SET(local, "instpntr", EVAL_REF, ref=i0);

    ndl_graph_print(graph);

    int64_t pid = ndl_runtime_proc_init(runtime, local);
    if (pid < 0) {
        fprintf(stderr, "Failed to create process.\n");
        exit(EXIT_FAILURE);
    }

    int err = ndl_runtime_proc_resume(runtime, pid);
    if (err != 0) {
        fprintf(stderr, "Failed to resume process.\n");
        exit(EXIT_FAILURE);
    }

    printf("[%3ld] Process started. Instruction@frame: %3d@%03d.\n", pid, i0, local);

    err = ndl_runtime_run_for(runtime, -1);
    if (err != 0) {
        fprintf(stderr, "Failed to run to infinity.\n");
        exit(EXIT_FAILURE);
    }

    ndl_runtime_print(runtime);
    ndl_graph_print(graph);

    printf("Testing GC and infinite loop.\n");
    SET(i3, "next    ", EVAL_REF, ref=i0);
    SET(local, "instpntr", EVAL_REF, ref=NDL_NULL_REF);
    ndl_graph_clean(graph);
    ndl_ref local2 = ndl_graph_salloc(graph, local, NDL_SYM("local2  "));
    SET(local2, "instpntr", EVAL_REF, ref=i0);
    ndl_graph_print(graph);

    pid = ndl_runtime_proc_init(runtime, local2);
    ndl_runtime_proc_resume(runtime, pid);
    printf("[%3ld] Process started. Instruction@frame: %3d@%03d.\n", pid, i0, local);

    ndl_runtime_run_for(runtime, -1);
    ndl_runtime_print(runtime);
    ndl_graph_print(graph);

    ndl_runtime_kill(runtime);

    return 0;
}

/* Run the canonical test fibonacci program.
 * Will be *much* more readable once constant symbols are set up for most operations.
 * Can't wait for that assembler. :P
 *
 * Local block starts with
 * self.arg1 = number of iterations.
 *
 * zero = 0
 * dec = -1
 *
 * a = 0
 * b = 1
 * branch count zero -> exit exit printb
 *
 * printb:
 * print b
 * a = a + b
 * count = count - dec
 * branch count zero -> exit exit printa
 *
 * printa:
 * print a
 * b = a + b
 * count = count - dec
 * branch count zero -> exit exit printb
 *
 * exit:
 * exit
 */
#define OPCODE(node, opcode)                              \
    SET(node, "opcode  ", EVAL_SYM, sym=NDL_SYM(opcode))
    
#define AOPCODE(node, opcode, a)                          \
    OPCODE(node, opcode);                                 \
    SET(node, "syma    ", EVAL_SYM, sym=NDL_SYM(a))

#define ABOPCODE(node, opcode, a, b)                      \
    AOPCODE(node, opcode, a);                             \
    SET(node, "symb    ", EVAL_SYM, sym=NDL_SYM(b))

#define ABCOPCODE(node, opcode, a, b, c)                  \
    ABOPCODE(node, opcode, a, b);                         \
    SET(node, "symc    ", EVAL_SYM, sym=NDL_SYM(c))

static int testruntimefibo(int steps, const char *path) {

    printf("Beginning fibonacci runtime tests.\n");

    ndl_runtime *runtime = ndl_runtime_init(NULL);

    if (runtime == NULL) {
        fprintf(stderr, "Failed to allocate runtime.\n");
        exit(EXIT_FAILURE);
    }

    ndl_graph *graph = ndl_runtime_graph(runtime);

    int instid = 0;
    ndl_ref insts[64];

    insts[instid] = testgraphalloc(graph);
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "a       ");
    SET(insts[instid], "const   ", EVAL_INT, num=0);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "b       ");
    SET(insts[instid], "const   ", EVAL_INT, num=1);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "zero    ");
    SET(insts[instid], "const   ", EVAL_INT, num=0);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "dec     ");
    SET(insts[instid], "const   ", EVAL_INT, num=1);
    instid++;

    int brancha = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABOPCODE(insts[instid], "branch  ", "arg1    ", "zero    ");
    instid++;

    int printb = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("gt      "));
    AOPCODE(insts[instid], "print   ", "b       ");
    instid++;
    
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "add     ", "a       ", "b       ", "a       ");
    instid++;
    
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "sub     ", "arg1    ", "dec     ", "arg1    ");
    instid++;

    int branchb = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABOPCODE(insts[instid], "branch  ", "arg1    ", "zero    ");
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("gt      "));
    AOPCODE(insts[instid], "print   ", "a       ");
    instid++;
    
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "add     ", "a       ", "b       ", "b       ");
    instid++;
    
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "sub     ", "arg1    ", "dec     ", "arg1    ");
    instid++;

    int branchc = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABOPCODE(insts[instid], "branch  ", "arg1    ", "zero    ");
    SET(insts[instid], "gt      ", EVAL_REF, ref=insts[printb]);
    instid++;

    int exit = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("lt      "));
    OPCODE(insts[instid], "exit    ");

    SET(insts[brancha], "lt      ", EVAL_REF, ref=insts[exit]);
    SET(insts[brancha], "eq      ", EVAL_REF, ref=insts[exit]);

    SET(insts[branchb], "lt      ", EVAL_REF, ref=insts[exit]);
    SET(insts[branchb], "eq      ", EVAL_REF, ref=insts[exit]);

    SET(insts[branchc], "eq      ", EVAL_REF, ref=insts[exit]);

    ndl_ref local = testgraphalloc(graph);
    SET(local, "instpntr", EVAL_REF, ref=insts[0]);
    SET(local, "arg1    ", EVAL_INT, num=steps);

    int64_t pid = ndl_runtime_proc_init(runtime, local);
    ndl_runtime_proc_resume(runtime, pid);

    printf("[%3ld] Process started. Instruction@frame: %3d@%03d.\n", pid, insts[0], local);

    ndl_runtime_print(runtime);

    ndl_runtime_run_for(runtime, -1);

    ndl_runtime_print(runtime);

    ndl_graph_print(graph);


    if (path != NULL) {

        printf("Saving graph in file %s.\n", path);

        int64_t est = ndl_graph_mem_est(graph);
        if (est < 0) {
            fprintf(stderr, "Failed to estimate serialized graph size.\n");
            ndl_runtime_kill(runtime);
            return -1;
        }

        char *buff = malloc((uint64_t) est);
        if (buff == NULL) {
            fprintf(stderr, "Failed to allocate serialization buffer.\n");
            ndl_runtime_kill(runtime);
            return -1;
        }

        int64_t size = ndl_graph_to_mem(graph, (uint64_t) est, buff);

        printf("Used %ld bytes to serialize file. Guessed: %ld.\n", size, est);

        if (size <= 0) {
            fprintf(stderr, "Failed to serialize graph.\n");
            ndl_runtime_kill(runtime);
            return -1;
        }

        FILE *out = fopen(path, "w");

        if (out == NULL) {
            fprintf(stderr, "Failed to open file.\n");
            ndl_runtime_kill(runtime);
            return -1;
        }
            
        uint64_t written;
        do {
            written = fwrite(buff, sizeof(char), (uint64_t) size, out);
            if (written > 0)
                size = size - (int64_t) written;
        } while ((size > 0) && (written > 0));
        fclose(out);
    }

    ndl_runtime_kill(runtime);

    return 0;
}

typedef struct kv_pair_s {

    uint8_t key[8];
    uint64_t value;
} kv_pair;

static int testslab(void) {

    printf("Beginning slab tests.\n");

    ndl_slab *slab = ndl_slab_init(sizeof(kv_pair), NDL_NULL_INDEX);

    if (slab == NULL) {
        fprintf(stderr, "Failed to allocate slab.\n");
        exit(EXIT_FAILURE);
    }

    printf("Slab (used+unused)/size: (%ld+%ld)/%ld.\n",
           ndl_slab_size(slab),
           ndl_slab_cap(slab) - ndl_slab_size(slab),
           ndl_slab_cap(slab));

    ndl_slab_index a = ndl_slab_head(slab);
    ndl_slab_index b = ndl_slab_next(slab, a);
    printf("Slab head, next: %ld, %ld.\n", a, b);

    ndl_slab_index c = ndl_slab_alloc(slab);
    a = ndl_slab_head(slab);
    b = ndl_slab_next(slab, a);
    printf("Allocated, head, next: %ld, %ld, %ld.\n", c, a, b);

    ndl_slab_index d = ndl_slab_alloc(slab);
    kv_pair *kva = ndl_slab_get(slab, c);
    kv_pair *kvb = ndl_slab_get(slab, d);
    printf("Allocated location: %p, %p.\n", (void *) kva, (void *) kvb);
    ndl_slab_print(slab);

    printf("Allocating 4ki nodes.\n");
    uint64_t i;
    for (i = 0; i < 4096; i++)
        ndl_slab_alloc(slab);

    printf("Freeing node.\n");
    ndl_slab_free(slab, d);

    printf("Slab (used+unused)/size: (%ld+%ld)/%ld.\n",
           ndl_slab_size(slab),
           ndl_slab_cap(slab) - ndl_slab_size(slab),
           ndl_slab_cap(slab));
    printf("Got %ld after freeing %ld.\n", ndl_slab_alloc(slab), d);

    printf("Freeing 4k nodes.\n");
    for (i = 0; i < 4000; i++)
        ndl_slab_free(slab, i + 10);
    printf("Slab (used+unused)/size: (%ld+%ld)/%ld.\n",
           ndl_slab_size(slab),
           ndl_slab_cap(slab) - ndl_slab_size(slab),
           ndl_slab_cap(slab));
    ndl_slab_print(slab);

    printf("Allocating 2k nodes.\n");
    for (i = 0; i < 4096; i++)
        ndl_slab_alloc(slab);

    printf("Slab (used+unused)/size: (%ld+%ld)/%ld.\n",
           ndl_slab_size(slab),
           ndl_slab_cap(slab) - ndl_slab_size(slab),
           ndl_slab_cap(slab));

    ndl_slab_kill(slab);

    return 0;
}

static int testruntimefork(int threads) {

    printf("Beginning fork tests.\n");

    ndl_runtime *runtime = ndl_runtime_init(NULL);

    if (runtime == NULL) {
        fprintf(stderr, "Failed to allocate runtime.\n");
        exit(EXIT_FAILURE);
    }

    ndl_graph *graph = ndl_runtime_graph(runtime);

    int instid = 0;
    ndl_ref insts[64];

    /* count = threads
     * one = 1
     * thrdfunc = REF(threadfunc)
     *
     * loop:
     * count --;
     * branch count one -> exit, next, next
     * invoke = new node()
     * invoke.instpntr = thrdfunc
     * invoke.id = count
     * fork invoke | next=REF(loop)
     *
     * threadfunc:
     * print count | next=REF(threadfunc)
     *
     * exit:
     * exit
     *
     */

    insts[instid] = testgraphalloc(graph);
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "count   ");
    SET(insts[instid], "const   ", EVAL_INT, num=threads);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "one     ");
    SET(insts[instid], "const   ", EVAL_INT, num=1);
    instid++;

    int thrdload = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "load    ", "instpntr", "const   ", "thrdfunc");
    /* SET(insts[instid], "const   ", EVAL_REF, ref=thrdfunc); */
    instid++;

    int base = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "sub     ", "count   ", "one     ", "count   ");
    instid++;

    int branch = instid;
    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABOPCODE(insts[instid], "branch  ", "count   ", "one     ");
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("eq      "));
    AOPCODE(insts[instid], "new     ", "invoke  ");
    SET(insts[instid - 1], "gt      ", EVAL_REF, ref=insts[instid]);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "save    ", "thrdfunc", "instpntr", "invoke  ");
    SET(insts[instid], "const   ", EVAL_INT, num=0);
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    ABCOPCODE(insts[instid], "save    ", "count   ", "id      ", "invoke  ");
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[instid - 1], NDL_SYM("next    "));
    AOPCODE(insts[instid], "fork    ", "invoke  ");
    SET(insts[instid], "next    ", EVAL_REF, ref=insts[base]);
    instid++;

    
    insts[instid] = ndl_graph_salloc(graph, insts[branch], NDL_SYM("lt      "));
    OPCODE(insts[instid], "exit    ");
    instid++;

    insts[instid] = ndl_graph_salloc(graph, insts[thrdload], NDL_SYM("const   "));
    AOPCODE(insts[instid], "print   ", "id      ");
    SET(insts[instid], "next    ", EVAL_REF, ref=insts[instid]);

    ndl_ref rootlocal = testgraphalloc(graph);
    SET(rootlocal, "instpntr", EVAL_REF, ref=insts[0]);

    int64_t pid = ndl_runtime_proc_init(runtime, rootlocal);
    ndl_runtime_proc_resume(runtime, pid);

    printf("[%3ld] Process started. Instruction@frame: %3d@%03d.\n", pid, insts[0], rootlocal);

    ndl_runtime_print(runtime);

    ndl_runtime_run_for(runtime, 10000);

    ndl_runtime_print(runtime);

    ndl_graph_print(graph);

    ndl_runtime_kill(runtime);

    return 0;
}

static int testgraphsave(void) {

    printf("Beginning serialization tests.\n");

    ndl_graph *graph = ndl_graph_init();

    ndl_ref n0 = testgraphalloc(graph);
    ndl_ref n1 = ndl_graph_salloc(graph, n0, NDL_SYM("bleh    "));
    ndl_ref n2 = ndl_graph_salloc(graph, n1, NDL_SYM("blah    "));
    ndl_ref n3 = ndl_graph_salloc(graph, n2, NDL_SYM("bluh    "));
    ndl_ref n4 = ndl_graph_salloc(graph, n3, NDL_SYM("blih    "));
    ndl_ref n5 = ndl_graph_salloc(graph, n4, NDL_SYM("bloh    "));
    ndl_ref n6 = ndl_graph_salloc(graph, n5, NDL_SYM("next    "));
    ndl_graph_salloc(graph, n6, NDL_SYM("ahoi    "));
    SET(n0, "hello   ", EVAL_SYM, sym=NDL_SYM("load    "));
    SET(n0, "numbah  ", EVAL_INT, num=37);
    SET(n1, "NEAXT   ", EVAL_REF, ref=n5);
    SET(n2, "FLOATER ", EVAL_FLOAT, real=3.14159);
    SET(n4, "const   ", EVAL_INT, num=2);
    SET(n6, "PNTR    ", EVAL_REF, ref=n3);
    SET(n3, "WOP     ", EVAL_REF, ref=n1);

    int64_t est = ndl_graph_mem_est(graph);
    if (est < 0) {
        fprintf(stderr, "Failed to estimate serialized graph size.\n");
        ndl_graph_kill(graph);
        return -1;
    }

    char buff[est];

    int64_t size = ndl_graph_to_mem(graph, (uint64_t) est, buff);

    printf("Est and return value: %ld, %ld.\n", est, size);

    ndl_graph_print(graph);

    ndl_graph_kill(graph);

    graph = ndl_graph_from_mem((uint64_t) size, (void *) buff);

    printf("Loading the serialized graph.\n");

    ndl_graph_print(graph);

    ndl_graph_kill(graph);

    return 0;
}

static int testhashtable(void) {

    printf("Beginning hashtable tests.\n");

    ndl_hashtable *table = ndl_hashtable_init(sizeof(int), sizeof(int), 8);
    ndl_hashtable_print(table);

    printf("Size needed by int->int 16 slot hashtable: %ld.\n",
           ndl_hashtable_msize(sizeof(int), sizeof(int), 16));

    printf("First key in hashtable: %p.\n",
           ndl_hashtable_keyhead(table));

    printf("First val in hashtable: %p.\n",
           ndl_hashtable_valhead(table));

    printf("Inserting a couple pairs.\n");

    int a=3, b=6;

    printf("New item: %p.\n", ndl_hashtable_put(table, &a, &b));

    a ++; b ++;
    printf("New item: %p.\n", ndl_hashtable_put(table, &a, &b));

    a ++; b ++;
    printf("New item: %p.\n", ndl_hashtable_put(table, &a, &b));
    ndl_hashtable_print(table);

    a = 4;
    int *c = ndl_hashtable_get(table, &a);
    printf("Got %d for 4.\n", *c);

    printf("All pairs:\n");
    c = ndl_hashtable_keyhead(table);
    while (c != NULL) {

        printf("Pair: %d:%d.\n", *c, *(c + 1));
        c = ndl_hashtable_keynext(table, c);
    }

    ndl_hashtable_kill(table);

    return 0;
}

static int testrehashtable(void) {

    printf("Beginning rehashtable tests.\n");

    ndl_rhashtable *table = ndl_rhashtable_init(sizeof(int), sizeof(int), 8);
    ndl_rhashtable_print(table);

    printf("Size needed by int->int 16 slot rehashtable: %ld.\n",
           ndl_rhashtable_msize(sizeof(int), sizeof(int), 16));

    printf("First key in rehashtable: %p.\n",
           ndl_rhashtable_keyhead(table));

    printf("First val in rehashtable: %p.\n",
           ndl_rhashtable_valhead(table));

    printf("Inserting a couple pairs.\n");

    int a=3, b=6;

    printf("New item: %p.\n", ndl_rhashtable_put(table, &a, &b));

    a ++; b ++;
    printf("New item: %p.\n", ndl_rhashtable_put(table, &a, &b));

    a ++; b ++;
    printf("New item: %p.\n", ndl_rhashtable_put(table, &a, &b));
    ndl_rhashtable_print(table);

    a = 4;
    int *c = ndl_rhashtable_get(table, &a);
    printf("Got %d for 4.\n", *c);

    printf("All pairs:\n");
    c = ndl_rhashtable_keyhead(table);
    while (c != NULL) {

        printf("Pair: %d:%d.\n", *c, *(c + 1));
        c = ndl_rhashtable_keynext(table, c);
    }

    printf("Inserting 100 items with collisions.\n");

    a = b = 0;
    int i;
    for (i = 0; i < 100; i++) {
        a = i + (a * 31) % 17;
        b = a + (b * 23) % 13;
        ndl_rhashtable_put(table, &a, &b);
    }
    ndl_rhashtable_print(table);

    printf("Removing 60 items with collisions.\n");
    a = b = 0;
    for (i = 0; i < 70; i++) {
        a = i + (a * 31) % 17;
        b = a + (b * 23) % 13;
        ndl_rhashtable_del(table, &a);
    }
    ndl_rhashtable_print(table);

    printf("Printing entire hashtable.\n");
    int *key = ndl_rhashtable_keyhead(table);
    while (key != NULL) {
        printf("key:value | %d:%d.\n", *key, *(key+1));
        key = ndl_rhashtable_keynext(table, key);
    }

    ndl_rhashtable_kill(table);

    return 0;
}

static int testassembler(void) {

    printf("Beginning assembly tests.\n");

    char *src1 =
        "loop:   \t          \r\n"
        "\tloop2:   #Labels.   \n"
        "add a, b\t-> c #WOOOO \n"
        "sub l.q->n            \n"
        "add a, 10 -> b        \n"
        "ollo :bleh            \n"
        "ollo :bleh            \n"
        "add a, 10.3 -> b      \n"
        "bleh:                 \n"
        "add b, -10 -> b       \n"
        "add b, -10.3 -> b     \n"
        "load :bleh, syma -> b \n";

    ndl_asm_parse_res res = ndl_asm_parse(src1, NULL);
    if (res.msg != NULL) {
        ndl_asm_print_err(res);
        exit(EXIT_FAILURE);
    }

    ndl_graph_print(res.graph);

    ndl_graph_kill(res.graph);

    return 0;
}

/* Run the canonical test fibonacci program,
 * using the assembler.
 *
 * Local block starts with
 * self.arg1 = number of iterations.
 *
 * branch count zero | lt=:exit eq=:exit gt=:printb symb=0
 *
 * printb:
 * print b
 * add a, b -> a
 * sum count, one -> count | symb=1
 * branch count zero | lt=:exit eq=:exit gt=:printa symb=0
 *
 * printa:
 * print a
 * add a, b -> b
 * sum count, one -> count | symb=1
 * branch count zero | lt=:exit eq=:exit gt=:printb symb=0
 *
 * exit:
 * exit
 */
static int testassemblerfibo(int count) {

    printf("Beginning fibo assembly test.\n");

    char *fibo =
        "branch count, 0 | lt=:exit eq=:exit gt=:printb \n"
        "printb:                                        \n"
        "print b                                        \n"
        "add a, b -> a                                  \n"
        "sum count, one -> count | symb=1               \n"
        "branch count, 0 | lt=:exit eq=:exit gt=:printa \n"
        "printa:                                        \n"
        "print a                                        \n"
        "add a, b -> b                                  \n"
        "sum count, one -> count | symb=1               \n"
        "branch count, 0 | lt=:exit eq=:exit gt=:printb \n"
        "exit:                                          \n"
        "exit                                           \n";

    ndl_asm_parse_res res = ndl_asm_parse(fibo, NULL);
    if (res.msg != NULL) {
        ndl_asm_print_err(res);
        exit(EXIT_FAILURE);
    }
    
    ndl_graph *rungraph = res.graph;

    ndl_graph_print(rungraph);

    ndl_runtime *runtime = ndl_runtime_init(rungraph);
    if (runtime == NULL) {
        fprintf(stderr, "Failed to create runtime.\n");
        exit(EXIT_FAILURE);
    }

    ndl_ref local = ndl_graph_alloc(rungraph);
    if (local == NDL_NULL_REF) {
        fprintf(stderr, "Failed to create local node.\n");
        exit(EXIT_FAILURE);
    }

    int err = 0;
    err |= ndl_graph_set(rungraph, local, NDL_SYM("instpntr"), NDL_VALUE(EVAL_REF, ref=res.head));
    err |= ndl_graph_set(rungraph, local, NDL_SYM("arg1    "), NDL_VALUE(EVAL_INT, ref=count));
    if (err != 0) {
        fprintf(stderr, "Failed to initialize local block.\n");
        exit(EXIT_FAILURE);
    }

    int64_t pid = ndl_runtime_proc_init(runtime, local);
    if (pid < 0) {
        fprintf(stderr, "Failed to create process.\n");
        exit(EXIT_FAILURE);
    }

    err = ndl_runtime_proc_resume(runtime, pid);
    if (err != 0) {
        fprintf(stderr, "Failed to resume process.\n");
        exit(EXIT_FAILURE);
    }

    ndl_runtime_print(runtime);

    ndl_runtime_run_for(runtime, count * 10);

    ndl_runtime_print(runtime);

    ndl_graph_print(rungraph);

    ndl_runtime_kill(runtime);
    ndl_graph_kill(rungraph);

    return 0;
}

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s testid\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Beginning tests.\n");

    int test = atoi(argv[1]);

    int err = 0;
    switch (test) {
    case 0:
        err = testprettyprint();
        break;
    case 1:
        err = testgraph();
        break;
    case 2:
        err = testruntimeadd();
        break;
    case 3:
        err = testruntimefork(10);
        break;
    case 4:
        err = testgraphsave();
        break;
    case 5:
        err = testslab();
        break;
    case 6:
        err = testhashtable();
        break;
    case 7:
        err = testrehashtable();
        break;
    case 8:
        if (argc >= 3)
            err = testruntimefibo(10, argv[2]);
        else
            err = testruntimefibo(10, NULL);
        break;
    case 9:
        if (argc >= 3)
            err = testruntimefibo(atoi(argv[2]), NULL);
        else
            err = testruntimefibo(1000000, NULL);
        break;
    case 10:
        err = testassembler();
        break;
    case 11:
        err = testassemblerfibo(10);
        break;
    default:
        fprintf(stderr, "Unknown test: %d. Valid indices: 0-8.\n", test);
        err = 1;
        break;
    }

    printf("Finished tests.\n");

    return err;
}
