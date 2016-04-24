/** nodel/src/nodeldump.c: Toy utility for dumping nodel graphs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "graph.h"
#include "asm.h"
#include "vector.h"

static void print_usage(void) {
    fprintf(stderr, "Usage: ndlasm graph.ndl\n");
    exit(EXIT_FAILURE);
}

#define DUMPER_BUFF_SIZE 4096

static ndl_vector *dump_load(FILE *in) {

    ndl_vector *graph = ndl_vector_init(sizeof(char));
    if (graph == NULL)
        return NULL;

    char buff[DUMPER_BUFF_SIZE];

    size_t sum = 0;
    while (!feof(in)) {

        size_t read = fread(buff, sizeof(char), DUMPER_BUFF_SIZE, in);

        if (read > 0) {
            void *res = ndl_vector_insert_range(graph, sum, read, buff);
            if (res == NULL) {
                ndl_vector_kill(graph);
                fprintf(stderr, "Failed to load graph.\n");
                return NULL;
            }

            sum += read;
        }

        if (ferror(in)) {
            ndl_vector_kill(graph);
            fprintf(stderr, "Failed to read input file: %s.\n", strerror(errno));
            return NULL;
        }
    }

    return graph;
}

static ndl_graph *dump_frommem(ndl_vector *src) {

    void *start = ndl_vector_get(src, 0);
    uint64_t len = ndl_vector_size(src);
    if (start == NULL) {
        fprintf(stderr, "Failed to get vector head.\n");
        return NULL;
    }

    ndl_graph *ret = ndl_graph_from_mem(len, start);
    if (ret == NULL) {
        fprintf(stderr, "Failed to load graph from memory.\n");
        return NULL;
    }

    return ret;
}

static int dump(FILE *in) {

    ndl_vector *graphvec = dump_load(in);
    if (graphvec == NULL)
        return -1;

    ndl_graph *graph = dump_frommem(graphvec);
    if (graph == NULL) {
        ndl_vector_kill(graphvec);
        return -1;
    }

    ndl_graph_print(graph);

    ndl_vector_kill(graphvec);
    ndl_graph_kill(graph);

    return 0;
}

int main(int argc, const char *argv[]) {

    if (argc != 2)
        print_usage();

    FILE *in;

    if (strcmp(argv[1], "-"))
        in = fopen(argv[1], "rb");
    else
        in = stdin;
    if (in == NULL) {
        fprintf(stderr, "Failed to open source file '%s': %s.\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Loading and printing source graph.\n");

    int err = dump(in);
    if (err != 0)
        exit(EXIT_FAILURE);

    if (in != stdin)
        fclose(in);

    return 0;
}
