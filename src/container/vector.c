#include "vector.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define max(a, b) ((a < b)? b : a)

ndl_vector *ndl_vector_init(uint64_t elem_size) {

    void *region = malloc(ndl_vector_msize(elem_size));
    if (region == NULL)
        return NULL;

    ndl_vector *ret = ndl_vector_minit(region, elem_size);
    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_vector_kill(ndl_vector *vector) {

    if (vector == NULL)
        return;

    ndl_vector_mkill(vector);

    free(vector);
}

ndl_vector *ndl_vector_minit(void *region, uint64_t elem_size) {

    ndl_vector *ret = (ndl_vector *) region;
    if (ret == NULL)
        return NULL;

    ret->elem_count = ret->elem_cap = 0;
    ret->elem_size = elem_size;

    ret->data = NULL;

    return ret;
}

void ndl_vector_mkill(ndl_vector *vector) {

    if (vector == NULL)
        return;

    if (vector->data != NULL)
        free(vector->data);

    return;
}

uint64_t ndl_vector_msize(uint64_t elem_size) {

    return sizeof(ndl_vector);
}

void *ndl_vector_get(ndl_vector *vector, uint64_t index) {

    if (vector == NULL)
        return NULL;

    if (index >= vector->elem_count)
        return NULL;

    return (void *) (vector->data + (index * vector->elem_size));
}

static inline void ndl_vector_shrink(ndl_vector *vector, int64_t delta) {

    uint64_t elem_count = vector->elem_count;
    uint64_t elem_cap = vector->elem_cap;

    if (elem_count + (uint64_t) (-delta) >= (elem_cap >> 2))
        return;

    uint64_t ncap = (elem_cap >> 2);

    void *ndata = realloc(vector->data, (size_t) ncap);
    if (ndata == NULL)
        return;

    vector->data = ndata;
    vector->elem_cap = ncap;

    return;
}

static inline int ndl_vector_grow(ndl_vector *vector, int64_t delta) {

    uint64_t elem_count = vector->elem_count;
    uint64_t elem_cap = vector->elem_cap;

    if (elem_count + (uint64_t) delta <= elem_cap)
        return 0;

    uint64_t ncap = max((elem_cap << 2), 4);

    void *ndata = realloc(vector->data, (size_t) (ncap * vector->elem_size));
    if (ndata == NULL)
        return -1;

    vector->data = ndata;
    vector->elem_cap = ncap;

    return 0;
}

static inline int ndl_vector_delta(ndl_vector *vector, int64_t delta) {

    if (delta > 0) {

        int err = ndl_vector_grow(vector, delta);
        if (err != 0)
            return err;

    } else if (delta < 0) {

        ndl_vector_shrink(vector, delta);
    }

    vector->elem_count = (uint64_t) ((int64_t) (vector->elem_count) + delta);

    return 0;
}

void *ndl_vector_push(ndl_vector *vector, void *elem) {

    int err = ndl_vector_delta(vector, 1);
    if (err != 0)
        return NULL;

    void *pos = (void *) (vector->data + ((vector->elem_count - 1) * vector->elem_size));

    memcpy(pos, elem, vector->elem_size);

    return pos;
}

int ndl_vector_pop(ndl_vector *vector) {

    ndl_vector_delta(vector, -1);

    return 0;
}

int ndl_vector_delete(ndl_vector *vector, uint64_t index) {

    if (index >= vector->elem_count)
        return -1;

    void *dst = (void *) (vector->data + (index * vector->elem_size));
    void *src = (void *) (vector->data + ((index + 1) * vector->elem_size));

    memmove(dst, src, (size_t) (vector->elem_count - index - 1));

    ndl_vector_delta(vector, -1);

    return 0;
}

void *ndl_vector_insert(ndl_vector *vector, uint64_t index, void *elem) {

    if (index > vector->elem_count)
        return NULL;

    int err = ndl_vector_delta(vector, 1);
    if (err != 0)
        return NULL;

    void *dst = (void *) (vector->data + ((index + 1) * vector->elem_size));
    void *src = (void *) (vector->data + (index * vector->elem_size));

    memmove(dst, src, (size_t) (vector->elem_count - index - 1));

    memcpy(src, elem, (size_t) vector->elem_size);

    return (void *) (vector->data + (index * vector->elem_size));
}

int ndl_vector_delete_range(ndl_vector *vector, uint64_t index, uint64_t len) {

    if (index + len > vector->elem_count)
        return -1;

    void *dst = (void *) (vector->data + (index * vector->elem_size));
    void *src = (void *) (vector->data + ((index + len) * vector->elem_size));

    memmove(dst, src, (size_t) (vector->elem_count - index - len));

    ndl_vector_delta(vector, - (int64_t) len);

    return 0;
}

void *ndl_vector_insert_range(ndl_vector *vector, uint64_t index, uint64_t len, void *elems) {

    if (index > vector->elem_count)
        return NULL;

    int err = ndl_vector_delta(vector, (int64_t) len);
    if (err != 0)
        return NULL;

    void *dst = (void *) (vector->data + ((index + len) * vector->elem_size));
    void *src = (void *) (vector->data + (index * vector->elem_size));

    memmove(dst, src, (size_t) (vector->elem_count - index - len));

    memcpy(src, elems, (size_t) (len * vector->elem_size));

    return (void *) (vector->data + (index * vector->elem_size));
}

uint64_t ndl_vector_size(ndl_vector *vector) {

    if (vector == NULL)
        return 0;

    return vector->elem_count;
}
uint64_t ndl_vector_cap (ndl_vector *vector) {

    if (vector == NULL)
        return 0;

    return vector->elem_cap;
}

uint64_t ndl_vector_elem_size(ndl_vector *vector) {

    if (vector == NULL)
        return 0;

    return vector->elem_size;
}
void ndl_vector_print(ndl_vector *vector) {

    printf("Printing vector.\n");
    if (vector == NULL) {
        printf("Vector is NULL.\n");
        return;
    }

    printf("elem_size: %ld.\n", vector->elem_size);
    printf("elem_count, cap: %ld, %ld.\n", vector->elem_count, vector->elem_cap);
    printf("Data pointer: %p.\n", vector->data);

    return;
}
