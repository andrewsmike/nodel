#include "test.h"

#include "slab.h"

/* Slabs are supposed to have parameter
 * invariant msize. Stuff (rehashtablepool)
 * relies on this.
 */
char *ndl_test_slab_msize(void) {

    uint64_t size = ndl_slab_msize(1, 1);
    if (ndl_slab_msize(10, 10) != size)
        return "Gave variable sizes";
    else if (ndl_slab_msize(100000000, 10000000) != size)
        return "Gave variable sizes";

    return NULL;
}

/* Test slab creation and deletion. */
char *ndl_test_slab_init(void) {

    ndl_slab *ret = ndl_slab_init(10, 10);
    if (ret == NULL)
        return "Failed to allocate";

    ndl_slab_kill(ret);

    return NULL;
}

/* Test in-place slab creation and deletion. */
char *ndl_test_slab_minit(void) {

    void *region = malloc(ndl_slab_msize(sizeof(int), 10));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_slab *ret = ndl_slab_minit(region, sizeof(int), 10);
    if (ret == NULL)
        return "Couldn't do in-place initialization";

    if (ret != region) {
        ndl_slab_mkill(ret);
        return "Messes with the pointer";
    }

    return NULL;
}

char *ndl_test_slab_it(void) {

    ndl_slab *ret = ndl_slab_init(sizeof(uint64_t), 10);
    if (ret == NULL)
        return "Failed to allocate slab";

    int i;
    for (i = 0; i < 100; i++) {
        ndl_slab_index index = ndl_slab_alloc(ret);
        if (index == NDL_NULL_INDEX) {
            ndl_slab_kill(ret);
            return "Failed to allocate from slab";
        }

        uint64_t *data = (uint64_t *) ndl_slab_get(ret, index);
        *data = index;
    }

    ndl_slab_index curr = ndl_slab_head(ret);
    for (i = 0; curr != NDL_NULL_INDEX; i++) {

        uint64_t *data = (uint64_t *) ndl_slab_get(ret, curr);
        if (*data != curr) {
            ndl_slab_kill(ret);
            return "Found bad content in slab element";
        }

        ndl_slab_free(ret, curr);

        curr = ndl_slab_next(ret, curr);
    }

    if (i != 100) {
        ndl_slab_print(ret);
        ndl_slab_kill(ret);
        printf("%d\n", i);
        return "Failed to delete all elements: iteration error";
    } else if (ndl_slab_size(ret) != 0) {
        ndl_slab_kill(ret);
        return "Failed to delete all elements";
    }

    return NULL;
}
