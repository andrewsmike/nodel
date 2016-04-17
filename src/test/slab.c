#include "test.h"

#include <stdio.h>

extern char *ndl_test_slab_init(void) {

    printf("Hello, world!\n");

    return NULL;
}

extern char *ndl_test_slab_kill(void) {

    printf("Goodbye, world!\n");

    return "Left the world";
}
