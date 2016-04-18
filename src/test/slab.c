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

char *ndl_test_slab_meta(void) {

    ndl_slab *slab = ndl_slab_init(sizeof(int), 10);
    if (slab == NULL)
        return "Couldn't allocate or initialize";

    return NULL;
}
