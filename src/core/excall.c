#include "excall.h"


#define KEYSIZE (sizeof(ndl_sym))
#define VALSIZE (sizeof(ndl_excall_func))
#define MINSIZE (32)

ndl_excall *ndl_excall_init(void) {

    return (ndl_excall *) ndl_rhashtable_init(KEYSIZE, VALSIZE, MINSIZE);
}

void ndl_excall_kill(ndl_excall *table) {

    ndl_rhashtable_kill((ndl_rhashtable *) table);
}

ndl_excall *ndl_excall_minit(void *region) {

    return (ndl_excall *) ndl_rhashtable_minit(region, KEYSIZE, VALSIZE, MINSIZE);
}

void ndl_excall_mkill(ndl_excall *table) {

    ndl_rhashtable_mkill((ndl_rhashtable *) table);
}

int64_t ndl_excall_msize(void) {

    return ndl_rhashtable_msize(KEYSIZE, VALSIZE, MINSIZE);
}

int ndl_excall_put(ndl_excall *table, ndl_sym name, ndl_excall_func func) {

    void *ret = ndl_rhashtable_put((ndl_rhashtable *) table, &name, &func);
    if (ret == NULL)
        return 1;
    else
        return 0;
}

ndl_excall_func ndl_excall_get(ndl_excall *table, ndl_sym name) {

    void *func = ndl_rhashtable_get((ndl_rhashtable *) table, &name);

    return *((ndl_excall_func *) func);
}

int ndl_excall_del(ndl_excall *table, ndl_sym name) {

    return ndl_rhashtable_del((ndl_rhashtable *) table, &name);
}

void *ndl_excall_head(ndl_excall *table) {

    return ndl_rhashtable_pairs_head((ndl_rhashtable *) table);
}

void *ndl_excall_next(ndl_excall *table, void *prev) {

    return ndl_rhashtable_pairs_next((ndl_rhashtable *) table, prev);
}

ndl_sym ndl_excall_key(ndl_excall *table, void *curr) {

    void *key = ndl_rhashtable_pairs_key((ndl_rhashtable *) table, curr);

    if (key == NULL)
        return NDL_NULL_SYM;

    return *((ndl_sym *) key)
}

ndl_excall_func ndl_excall_func(ndl_excall *table, void *curr) {

    void *func = ndl_rhashtable_pairs_val((ndl_rhashtable *) table, curr);

    return *((ndl_excall_func *) func);
}
