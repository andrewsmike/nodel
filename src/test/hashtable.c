#include "test.h"

#include "hashtable.h"

char *ndl_test_hashtable_alloc(void) {

    ndl_hashtable *table = ndl_hashtable_init(sizeof(int), sizeof(int), 8);
    if (table == NULL)
        return "Failed to allocate table";

    ndl_hashtable_kill(table);

    return NULL;
}

char *ndl_test_hashtable_minit(void) {

    void *region = malloc(ndl_hashtable_msize(sizeof(int), sizeof(int), 10));
    if (region == NULL)
        return "Out of memory, couldn't run test";

    ndl_hashtable *ret = ndl_hashtable_minit(region, sizeof(int), sizeof(int), 10);
    if (ret == NULL)
        return "Couldn't do in-place initialization";

    if (ret != region)
        return "Messes with the pointer";

    return NULL;
}

char *ndl_test_hashtable_it(void) {

    ndl_hashtable *table = ndl_hashtable_init(sizeof(int), sizeof(int), 8);
    if (table == NULL)
        return "Failed to allocate table";

    if (ndl_hashtable_msize(sizeof(int), sizeof(int), 16) != 224) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Required wrong amount of memory";
    }

    if (ndl_hashtable_keyhead(table) != NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Empty table gives valid key iterator.";
    }

    if (ndl_hashtable_valhead(table) != NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Empty table gives valid value iterator.";
    }

    void *data;

    int a=3, b=6;

    data = ndl_hashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Failed to put element";
    }

    a ++; b ++;
    data = ndl_hashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Failed to put element";
    }

    a ++; b ++;
    data = ndl_hashtable_put(table, &a, &b);
    if (data == NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Failed to put element";
    }

    a = 4;
    int *c = ndl_hashtable_get(table, &a);
    if (c == NULL) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Failed to get element";
    }
    if (*c != 7) {
        ndl_hashtable_print(table);
        ndl_hashtable_kill(table);
        return "Got wrong value for 4";
    }
    
    c = ndl_hashtable_keyhead(table);
    while (c != NULL) {

        if (c == NULL) {
            ndl_hashtable_print(table);
            ndl_hashtable_kill(table);
            return "Failed to get element";
        }
        if (*c != *(c + 1) - 3) {
            ndl_hashtable_print(table);
            ndl_hashtable_kill(table);
            return "Got wrong data";
        }
        c = ndl_hashtable_keynext(table, c);
    }

    ndl_hashtable_kill(table);

    return 0;
}
