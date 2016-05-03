/** nodel/src/nodelrun.c: Toy utility for running nodel graphs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "graph.h"
#include "runtime.h"
#include "endian.h"

/* nodel: Simple executable to load nodel graphs
 * and execute their programs.
 * "Because why not?!"
 */

#define FAIL(...)                     \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        exit(EXIT_FAILURE);           \
    } while (0)

#define FILE_BUFFER_SIZE (uint64_t) (2 << 16)

static inline ndl_value parse_sym(char *arg) {

    uint64_t len = strlen(arg);
    if (len > 8)
        FAIL("Symbol is too long: '%s. Must be eight characters or fewer.\n", arg);

    ndl_value ret = NDL_VALUE(EVAL_SYM, sym=NDL_SYM("        "));
    memcpy((char *) &ret.sym, arg, len);

    return ret;
}

static inline ndl_value parse_int(char *arg) {

    int64_t num;
    sscanf(arg, "%ld", &num);

    return NDL_VALUE(EVAL_INT, num=num);
}

static inline ndl_value parse_float(char *arg) {

    double real;
    sscanf(arg, "%lf", &real);

    return NDL_VALUE(EVAL_FLOAT, real=real);
}

static inline ndl_value parse_arg(char *arg) {

    if (arg[0] == '\'')
        return parse_sym(arg + 1);

    char *t;
    for (t = arg; *t != '\0'; t++) {
        if (*t == '.')
            return parse_float(arg);
    }

    return parse_int(arg);
}

int main(int argc, char *argv[]) {

    if (argc < 2)
        FAIL("Usage: %s [file] arg...\n", argv[0]);

    if (argc > 2 + 15)
        FAIL("Too many arguments. Usage: %s [file] arg...\n", argv[0]);

    char *buff = (char*) malloc(FILE_BUFFER_SIZE);
    if (buff == NULL)
        FAIL("Failed to allocate file buffer.\n");

    FILE *in;

    if (strcmp(argv[1], "-"))
        in = fopen(argv[1], "r");
    else
        in = stdin;
    if (in == NULL)
        FAIL("Failed to open file.\n");

    uint64_t curr = 0;
    uint64_t used = 1;

    while (used > 0) {
        used = fread(&buff[curr], sizeof(char), FILE_BUFFER_SIZE - curr, in);
        if (used > 0) curr += used;
    }

    if (in != stdin)
        fclose(in);

    ndl_graph *graph = ndl_graph_from_mem(curr, buff);
    if (graph == NULL)
        FAIL("Failed to load graph: Bad file format.\n");

    ndl_runtime *runtime = ndl_runtime_init(graph);

    ndl_ref local = ndl_graph_alloc(graph);
    ndl_graph_set(graph, local, NDL_SYM("instpntr"), NDL_VALUE(EVAL_REF, ref=(ndl_ref) 1));

    argc -= 2;

    char argname[8];
    memcpy(argname, "arg     ", 8);

    char *name = argname;

    int i;
    for (i = 0; i < argc; i++) {
        ndl_value arg = parse_arg(argv[i + 2]);
        argname[3] = (char) ((i < 9)? '1' + i : 'A' + i);
        ndl_sym key = NDL_SYM(name);
        ndl_graph_set(graph, local, key, arg);
    }

    ndl_proc *proc = ndl_runtime_proc_init(runtime, local, ndl_time_from_usec(100000));
    if (proc == NULL) {
        fprintf(stderr, "Failed to create process.\n");
        exit(EXIT_FAILURE);
    }

    ndl_proc_resume(proc);
    ndl_runtime_run_for(runtime, NDL_TIME_ZERO);

    printf("Exiting.\n");

    exit(EXIT_SUCCESS);
}
