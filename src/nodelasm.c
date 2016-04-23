/** nodel/src/nodelasm.c: Toy utility for assembling nodel program graphs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "graph.h"
#include "asm.h"
#include "vector.h"

static void print_usage(void) {
    fprintf(stderr, "Usage: ndlasm source.asm [-o output.ndl]\n");
    exit(EXIT_FAILURE);
}

#define ASSEMBLER_BUFF_SIZE 4096

static ndl_vector *assemble_load(FILE *in) {

    ndl_vector *code = ndl_vector_init(sizeof(char));
    if (code == NULL)
        return NULL;

    char buff[ASSEMBLER_BUFF_SIZE];

    size_t sum = 0;
    while (!feof(in)) {

        size_t read = fread(buff, sizeof(char), ASSEMBLER_BUFF_SIZE, in);

        if (read > 0) {
            void *res = ndl_vector_insert_range(code, sum, read, buff);
            if (res == NULL) {
                ndl_vector_kill(code);
                fprintf(stderr, "Failed to load code vector.\n");
                return NULL;
            }

            sum += read;
        }

        if (ferror(in)) {
            ndl_vector_kill(code);
            fprintf(stderr, "Failed to read input file: %s.\n", strerror(errno));
            return NULL;
        }
    }

    void *ret = ndl_vector_push(code, NULL);
    if (ret == NULL) {
        ndl_vector_kill(code);
        fprintf(stderr, "Failed to append NULL terminator to file.");
        return NULL;
    }

    return code;
}

static ndl_graph *assemble_assemble(ndl_vector *code) {

    char *src = (char *) ndl_vector_get(code, 0);
    if (src == NULL) {
        fprintf(stderr, "Failed to load source from code vector.\n");
        return NULL;
    }

    ndl_asm_result res = ndl_asm_parse(src, NULL);
    if (res.msg != NULL) {

        ndl_asm_print_err(res);

        if (res.graph != NULL)
            ndl_graph_kill(res.graph);

        return NULL;
    }

    ndl_graph *clean = ndl_graph_init();
    if (clean == NULL) {
        fprintf(stderr, "Failed to allocate graph.\n");
        return NULL;
    }

    ndl_ref roots[2] = {res.inst_head, NDL_NULL_REF};

    int err = ndl_graph_dcopy(clean, res.graph, roots);
    if (err != 0) {
        fprintf(stderr, "Failed to copy graph.\n");

        ndl_graph_kill(res.graph);
        ndl_graph_kill(clean);

        return NULL;
    }

    ndl_graph_kill(res.graph);

    return clean;
}

static ndl_vector *assemble_tomem(ndl_graph *src) {

    ndl_vector *bcode = ndl_vector_init(sizeof(char));
    if (bcode == NULL)
        return NULL;

    uint64_t size = 0;
    uint64_t add = (uint64_t) ndl_graph_mem_est(src);
    int64_t written;
    while (1) {
        void *ret = ndl_vector_insert_range(bcode, size, add, NULL);
        void *start = ndl_vector_get(bcode, 0);
        if ((ret == 0) || (start == NULL)) {
            ndl_vector_kill(bcode);
            fprintf(stderr, "Failed to grow bcode vector.\n");
            return NULL;
        }

        size += add;

        written = ndl_graph_to_mem(src, size, start);
        if (written > 0)
            break;

        add += add;
    }

    int err = ndl_vector_delete_range(bcode, (uint64_t) written, size - (uint64_t) written);
    if (err != 0) {
        ndl_vector_kill(bcode);
        fprintf(stderr, "Failed to fit bcode vector.\n");
        return NULL;
    }

    return bcode;
}

static int assemble_save(FILE *out, ndl_vector *bcode) {

    uint64_t offset = 0;
    uint64_t max = ndl_vector_size(bcode);

    char *curr = ndl_vector_get(bcode, 0);
    if (curr == NULL) {
        fprintf(stderr, "Failed to read from bcode vector.\n");
        return -1;
    }

    while (offset < max) {

        size_t written = fwrite(curr, sizeof(char), max - offset, out);
        if (written > 0) {
            offset += written;
            curr += written;
        } else {
            return -1;
        }
    }

    return 0;
}

static int assemble(FILE *out, FILE *in) {

    ndl_vector *code = assemble_load(in);
    if (code == NULL)
        return -1;

    ndl_graph *res = assemble_assemble(code);
    if (res == NULL) {
        ndl_vector_kill(code);
        return -1;
    }

    ndl_vector *bcode = assemble_tomem(res);
    if (bcode == NULL) {
        ndl_vector_kill(code);
        ndl_graph_kill(res);
        return -1;
    }

    int err = assemble_save(out, bcode);

    ndl_vector_kill(code);
    ndl_vector_kill(bcode);
    ndl_graph_kill(res);

    return err;
}

int main(int argc, const char *argv[]) {

    if ((argc == 1) || (argc == 3) || (argc > 4))
        print_usage();

    FILE *in, *out;

    in = fopen(argv[1], "rb");
    if (in == NULL) {
        fprintf(stderr, "Failed to open source file '%s': %s.\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    out = stdout;
    if (argc == 4) {
        if (strcmp(argv[2], "-o"))
            print_usage();

        out = fopen(argv[3], "wb");
        if (out == NULL) {
            fprintf(stderr, "Failed to open destination file '%s': %s.\n", argv[3], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    int err = assemble(out, in);
    if (err != 0) {
        fprintf(stderr, "Failed to assemble and save program.\n");
        exit(EXIT_FAILURE);
    }

    fclose(in);
    if (out != stdout)
        fclose(out);

    return 0;
}
