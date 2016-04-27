#include "test.h"

#include "rehashtable.h"
#include "test.h"

#include "hashtable.h"

char *ndl_test_rehashtable_alloc(void) {

    ndl_rhashtable *table = ndl_rhashtable_init(sizeof(int), sizeof(int), 8);
    if (table == NULL)
        return "Failed to allocate table";

    ndl_rhashtable_kill(table);

    return NULL;
}

char *ndl_test_rehashtable_minit(void) {

    void *region = malloc(ndl_rhashtable_msize(sizeof(int), sizeof(int), 10));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_rhashtable *ret = ndl_rhashtable_minit(region, sizeof(int), sizeof(int), 10);
    if (ret == NULL) {
        free(region);
        return "Couldn't do in-place initialization";
    }

    if (ret != region) {
        ndl_rhashtable_mkill(ret);
        free(region);
        return "Messes with the pointer";
    }

    ndl_rhashtable_mkill(ret);
    free(region);

    return NULL;
}

char *ndl_test_rehashtable_it(void) {

    ndl_rhashtable *table = ndl_rhashtable_init(sizeof(int), sizeof(int), 8);
    if (table == NULL)
        return "Failed to allocate table";

    if (ndl_rhashtable_msize(sizeof(int), sizeof(int), 16) != 16) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Required wrong amount of memory";
    }

    if (ndl_rhashtable_pairs_head(table) != NULL) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Empty table gives valid key iterator.";
    }

    void *data;

    int a=3, b=6;

    data = ndl_rhashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Failed to put element";
    }

    a ++; b ++;
    data = ndl_rhashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Failed to put element";
    }

    a ++; b ++;
    data = ndl_rhashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Failed to put element";
    }

    a = 4;
    int *c = ndl_rhashtable_get(table, &a);
    if (c == NULL) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Failed to get element";
    }
    if (*c != 7) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Got wrong value for 4";
    }
    
    c = ndl_rhashtable_pairs_head(table);
    while (c != NULL) {

        int *data = ndl_rhashtable_pairs_key(table, c);

        if (data == NULL) {
            ndl_rhashtable_print(table);
            ndl_rhashtable_kill(table);
            return "Failed to get element";
        }
        if (*data != *(data + 1) - 3) {
            ndl_rhashtable_print(table);
            ndl_rhashtable_kill(table);
            return "Got wrong data";
        }
        c = ndl_rhashtable_pairs_next(table, c);
    }

    ndl_rhashtable_kill(table);

    return 0;
}

char *ndl_test_rehashtable_volume(void) {

    ndl_rhashtable *table = ndl_rhashtable_init(sizeof(int), sizeof(int), 8);
    if (table == NULL)
        return "Failed to allocate table";

    int a, b;
    a = b = 0;

    int i;
    for (i = 0; i < 100; i++) {
        a = i * (a * 31) % 17;
        b = i * (b * 23) % 13;
        void *data = ndl_rhashtable_put(table, &a, &b);
        if (data == NULL) {
            ndl_rhashtable_kill(table);
            return "Failed to put item";
        }
    }

    a = b = 0;
    for (i = 0; i < 100; i++) {
        a = i + (a * 31) % 17;
        b = a + (b * 23) % 13;
        ndl_rhashtable_put(table, &a, &b);
    }

    if (ndl_rhashtable_size(table) != 66) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Found wrong number of items given current hash function";
    }

    a = b = 0;
    for (i = 0; i < 70; i++) {
        a = i + (a * 31) % 17;
        b = a + (b * 23) % 13;
        ndl_rhashtable_del(table, &a);
    }

    if (ndl_rhashtable_size(table) != 20) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Found wrong number of items given current hash function";
    }

    int *pair = ndl_rhashtable_pairs_head(table);
    for (i = 0; pair != NULL; i++) {
        pair = ndl_rhashtable_pairs_next(table, pair);
    }
    if (i != 20) {
        ndl_rhashtable_print(table);
        ndl_rhashtable_kill(table);
        return "Found wrong number of keys";
    }

    ndl_rhashtable_kill(table);

    return 0;
}
