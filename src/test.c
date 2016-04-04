/** nodel/src/test.c: Test implemented functionality. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "node.h"
#include "graph.h"
#include "runtime.h"

int testprettyprint(void) {

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

void testgraphprintnode(ndl_graph *graph, ndl_ref node) {

    int size = ndl_graph_size(graph, node);

    printf("Pairs: %d.\n", size);

    char symbuff[16];
    symbuff[15] = '\0';

    char valbuff[16];
    valbuff[15] = '\0';

    int i;
    for (i = 0; i < size; i++) {
        ndl_sym key = ndl_graph_index(graph, node, i);
        ndl_value val = ndl_graph_get(graph, node, key);

        ndl_value_to_string(NDL_VALUE(EVAL_SYM, sym=key), 15, symbuff);
        ndl_value_to_string(val, 15, valbuff);

        printf("(%s:%s)\n", symbuff, valbuff);
    }

    int count = ndl_graph_backref_size(graph, node);
    printf("Backrefs: %d\n", count);

    for (i = 0; i < count; i++) {
        ndl_ref back = ndl_graph_backref_index(graph, node, i);
        ndl_value_to_string(NDL_VALUE(EVAL_REF, ref=back), 15, valbuff);
        printf("'%s'.\n", valbuff);
    }
}

ndl_ref testgraphalloc(ndl_graph *graph) {

    ndl_ref ret = ndl_graph_alloc(graph);

    if (ret == NDL_NULL_REF) {
        fprintf(stderr, "Failed to allocate graph node.\n");
        exit(EXIT_FAILURE);
    }

    return ret;
}

void testgraphaddedge(ndl_graph *graph, ndl_ref a, ndl_ref b, const char *name) {

    int err = ndl_graph_set(graph, a, NDL_SYM(name), NDL_VALUE(EVAL_REF, ref=b));

    if (err != 0) {
        fprintf(stderr, "Failed to add edge\n");
        exit(EXIT_FAILURE);
    }
}

void testgraphdeledge(ndl_graph *graph, ndl_ref a, const char *name) {

    int err = ndl_graph_del(graph, a, NDL_SYM(name));

    if (err != 0) {
        fprintf(stderr, "Failed to delete edge\n");
        exit(EXIT_FAILURE);
    }
}

int testgraph(void) {

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

int testruntime(void) {

    printf("Beginning runtime tests.\n");

    ndl_runtime *runtime = ndl_runtime_init();

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
    ndl_ref i5 = ndl_graph_salloc(graph, i4, NDL_SYM("next    "));
    ndl_ref i6 = ndl_graph_salloc(graph, i5, NDL_SYM("next    "));
    ndl_graph_salloc(graph, i6, NDL_SYM("next    "));
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

    ndl_ref local = testgraphalloc(graph);
    SET(local, "instpntr", EVAL_REF, ref=i0);

    int pid = ndl_runtime_proc_init(runtime, local);

    printf("[%3d] Process started. Instruction@frame: %3d@%03d.\n", pid, i0, local);

    ndl_runtime_print(runtime);

    ndl_runtime_step(runtime, 1); ndl_runtime_print(runtime);
    ndl_runtime_step(runtime, 1); ndl_runtime_print(runtime);
    ndl_runtime_step(runtime, 1); ndl_runtime_print(runtime);
    ndl_runtime_step(runtime, 1); ndl_runtime_print(runtime);
    ndl_runtime_step(runtime, 1); ndl_runtime_print(runtime);

    ndl_graph_print(graph);


    ndl_runtime_kill(runtime);

    return 0;
}

int main(int argc, const char *argv[]) {

    printf("Beginning tests.\n");

    int err;
    err  = testprettyprint();
    //err |= testgraph();
    err |= testruntime();

    printf("Finished tests.\n");

    return err;
}
