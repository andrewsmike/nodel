/** nodel/src/test.c: Test implemented functionality. */

#include <stdio.h>
#include <stdlib.h>

#include "node.h"

int main(int argc, const char *argv[]) {

    printf("Hello, world.\n");

    ndl_node_pool *pool = ndl_node_pool_init();

    if (pool == NULL) {
        fprintf(stderr, "Failed to allocate nodepool. !?!?\n");
        exit(EXIT_FAILURE);
    }

    ndl_node_pool_kill(pool);
}
