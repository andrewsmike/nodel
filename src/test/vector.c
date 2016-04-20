#include "test.h"

#include "vector.h"

#include <stdio.h>
#include <string.h>


/* Size is invariant to elem_count. */
char *ndl_test_vector_msize(void) {

    uint64_t size = ndl_vector_msize(0);
    if (ndl_vector_msize(100) != size)
        return "Gave different sizes";
    else if (ndl_vector_msize(100000) != size)
        return "Gave different sizes";

    return NULL;
}

char *ndl_test_vector_init(void) {

    ndl_vector *vec = ndl_vector_init(sizeof(int));
    if (vec == NULL)
        return "Failed to allocate";

    ndl_vector_kill(vec);

    return NULL;
}

char *ndl_test_vector_minit(void) {

    void *region = malloc(ndl_vector_msize(sizeof(int)));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_vector *vec = ndl_vector_minit(region, sizeof(int));
    if (vec == NULL) {
        free(region);
        return "In-place initialization failed";
    }

    if (vec != region) {
        ndl_vector_mkill(vec);
        free(region);
        return "Messes with the pointer";
    }

    ndl_vector_mkill(vec);
    free(region);

    return NULL;
}

char *ndl_test_vector_stack(void) {

    ndl_vector *vec = ndl_vector_init(sizeof(int));
    if (vec == NULL)
        return "Failed to allocate";

    int i;
    for (i = 0; i < 200; i++) {
        void *t = ndl_vector_push(vec, &i);
        if (*((int *) t) != i) {
            ndl_vector_kill(vec);
            return "Failed to push element";
        }
    }

    if (ndl_vector_size(vec) != 200) {
        ndl_vector_kill(vec);
        return "Too few elements";
    }

    void *data = ndl_vector_get(vec, 199);
    if (data == NULL) {
        ndl_vector_kill(vec);
        return "Failed to get element";
    }

    if (*((int *) data) != 199) {
        ndl_vector_kill(vec);
        return "Got wrong argument";
    }

    for (i = 0; i < 175; i++) {
        int err = ndl_vector_pop(vec);
        if (err != 0) {
            ndl_vector_kill(vec);
            return "Failed to pop";
        }
    }

    if (ndl_vector_size(vec) != 25) {
        ndl_vector_kill(vec);
        return "Wrong number of elements";
    }

    ndl_vector_kill(vec);

    return NULL;
}

char *ndl_test_vector_region(void) {

    ndl_vector *vec = ndl_vector_init(sizeof(char));
    if (vec == NULL)
        return "Failed to allocate";

    char t = '\0';
    void *elem = ndl_vector_push(vec, &t);
    if (elem == NULL) {
        ndl_vector_kill(vec);
        return "Failed to push terminator";
    }

    t = 'a';

    elem = ndl_vector_insert(vec, 0, &t);
    if (elem == NULL) {
        ndl_vector_kill(vec);
        return "Failed to push 'a'";
    }

    t = 'b';

    elem = ndl_vector_insert(vec, 1, &t);
    if (elem == NULL) {
        ndl_vector_kill(vec);
        return "Failed to push 'b'";
    }

    char *data = (char *) ndl_vector_get(vec, 0);
    if (data == NULL) {
        ndl_vector_kill(vec);
        return "Failed to get elements";
    }

    if (data[0] != 'a' || data[1] != 'b' || data[2] != '\0') {
        ndl_vector_kill(vec);
        return "Got initial wrong contents";
    }

    int err = ndl_vector_delete(vec, 1);
    if (err != 0) {
        ndl_vector_kill(vec);
        return "Failed to delete element";
    }

    if (data[0] != 'a' || data[1] != '\0') {
        ndl_vector_kill(vec);
        return "Got wrong contents";
    }

    char *a = "_hello world_";

    data = (char *) ndl_vector_insert_range(vec, 1, strlen(a), a);
    if (data == NULL) {
        ndl_vector_kill(vec);
        return "Failed to range insert";
    }

    data = ndl_vector_get(vec, 0);
    if (data == NULL) {
        ndl_vector_kill(vec);
        return "Failed to get after range insert";
    }

    if (ndl_vector_size(vec) != 15) {
        ndl_vector_kill(vec);
        return "Bad count after range insert";
    }

    if (strncmp(data, "a_hello world_", 15) != 0) {
        ndl_vector_kill(vec);
        return "Bad contents after range insert";
    }

    err = ndl_vector_delete_range(vec, 2, strlen("hello"));
    if (err != 0) {
        ndl_vector_kill(vec);
        return "Failed to range delete";
    }

    data = (char *) ndl_vector_get(vec, 0);
    if (data == NULL) {
        ndl_vector_kill(vec);
        return "Failed to get contents after range delete";
    }

    if (strncmp(data, "a_ world_", 15 - strlen("hello")) != 0) {
        ndl_vector_kill(vec);
        return "Bad contents after range delete";
    }

    ndl_vector_kill(vec);
    return NULL;
}
