#include "slab.h"

#include <stdlib.h>
#include <stdio.h>

#define max(a, b) ((a > b)? a : b)

static inline ndl_slab_item *ndl_slab_get_item(ndl_slab *slab, ndl_slab_index index) {

    uint64_t bs = slab->block_size;

    uint64_t bindex = index / bs;
    uint64_t iindex = index % bs;
    uint64_t ioffset = iindex * (sizeof(ndl_slab_item) + slab->elem_size);

    return (ndl_slab_item*) (((uint8_t *) slab->blocks[bindex]) + ioffset);
}

/* When block_size not specified, aim for blocks of 1K bytes. */
#define NDL_SLAB_BLOCK_SIZE 1024

ndl_slab *ndl_slab_init(uint64_t elem_size, uint64_t block_size) {

    ndl_slab *slab = malloc(sizeof(ndl_slab));
    if (slab == NULL)
        return slab;

    ndl_slab *ret = ndl_slab_minit(slab, elem_size, block_size);
    if (ret == NULL)
        free(slab);

    return ret;
}

void ndl_slab_kill(ndl_slab *slab) {

    if (slab == NULL)
        return;

    ndl_slab_mkill(slab);

    free(slab);

    return;
}

ndl_slab *ndl_slab_minit(void *region, uint64_t elem_size, uint64_t block_size) {

    ndl_slab *slab = (ndl_slab *) region;

    if (slab == NULL)
        return NULL;

    slab->elem_size = elem_size;

    if (block_size != NDL_NULL_INDEX)
        slab->block_size = block_size;
    else
        slab->block_size = max(1, NDL_SLAB_BLOCK_SIZE / (sizeof(ndl_slab_item) + elem_size));

    slab->block_count = 0;
    slab->block_cap = 0;
    slab->elem_count = 0;

    slab->free_head = 0;

    slab->blocks = NULL;

    return slab;
}

void ndl_slab_mkill(ndl_slab *slab) {

    if (slab == NULL)
        return;

    uint64_t i;
    for (i = 0; i < slab->block_count; i++)
        free(slab->blocks[i]);

    if (slab->blocks != NULL)
        free(slab->blocks);

    return;
}

/* Currently, slab size does not depend on elem_size or
 * block_size. Should the unthinkable happen, and that change,
 * rehashtablepool depends on this. Fix that.
 */
uint64_t ndl_slab_msize(uint64_t elem_size, uint64_t block_size) {

    return sizeof(ndl_slab);
}

static inline void ndl_slab_grow(ndl_slab *slab) {

    if (slab->block_count == slab->block_cap) {

        uint64_t nsize = max(8, slab->block_count << 1);

        void *nblocks = realloc(slab->blocks, nsize * sizeof(ndl_slab_block*));
        if (nblocks == NULL)
            return;

        slab->blocks = nblocks;
        slab->block_cap = nsize;
    }

    uint64_t item_size = sizeof(ndl_slab_item) + slab->elem_size;
    ndl_slab_block *nblock = malloc(item_size * slab->block_size);

    if (nblock == NULL)
        return;

    uint64_t blockind = slab->block_count++;
    slab->blocks[blockind] = nblock;

    uint64_t base_index = blockind * slab->block_size;

    ndl_slab_item *curr = (ndl_slab_item *) nblock;

    uint64_t i;
    for (i = 0; i < slab->block_size; i++) {

        curr->next_free = i + base_index + 1;
        curr = (ndl_slab_item *) (((uint8_t *) curr) + item_size);
    }

    return;
}

ndl_slab_index ndl_slab_alloc(ndl_slab *slab) {

    if (slab == NULL)
        return NDL_NULL_INDEX;

    uint64_t block_count = slab->block_count;
    uint64_t size = slab->block_size * block_count;

    if (slab->free_head == size) {

        ndl_slab_grow(slab);

        if (slab->block_count == block_count)
            return NDL_NULL_INDEX;
    }

    ndl_slab_index curr = slab->free_head;
    ndl_slab_item *elem = ndl_slab_get_item(slab, curr);

    slab->free_head = elem->next_free;
    elem->next_free = NDL_NULL_INDEX;

    slab->elem_count ++;

    return curr;
}

void ndl_slab_free(ndl_slab *slab, ndl_slab_index index) {

    if ((slab == NULL) || (index == NDL_NULL_INDEX))
        return;

    if (index >= (slab->block_count * slab->block_size))
        return;

    ndl_slab_item *elem = ndl_slab_get_item(slab, index);

    elem->next_free = slab->free_head;
    slab->free_head = index;
    slab->elem_count--;

    return;
}

void *ndl_slab_get(ndl_slab *slab, ndl_slab_index index) {

    if (slab == NULL)
        return NULL;

    if (index >= (slab->block_count * slab->block_size))
        return NULL;

    return ((uint8_t *) ndl_slab_get_item(slab, index)) + sizeof(ndl_slab_item);
}

static inline ndl_slab_index ndl_slab_scan(ndl_slab *slab, ndl_slab_index start) {

    uint64_t size = (slab->block_count * slab->block_size);
    if (start >= size)
        return NDL_NULL_INDEX;

    ndl_slab_block **blocks = slab->blocks;

    uint64_t blockind = start / slab->block_size;
    uint64_t blockoff = start % slab->block_size;
    uint64_t itemsize = slab->elem_size + sizeof(ndl_slab_item);

    ndl_slab_item *curr = (ndl_slab_item *) (((uint8_t *) blocks[blockind]) + (itemsize * blockoff));

    while (1) {

        if (blockoff >= slab->block_size) {
            blockind++;
            blockoff = 0;
            curr = (ndl_slab_item *) blocks[blockind];
        }

        if (blockind >= slab->block_count) {
            return NDL_NULL_INDEX;
        }

        if (curr->next_free == NDL_NULL_INDEX)
            break;

        blockoff ++;
        curr = (ndl_slab_item *) (((uint8_t *) curr) + itemsize);
    }

    return (blockind * slab->block_size) + blockoff;
}

ndl_slab_index ndl_slab_head(ndl_slab *slab) {

    if (slab == NULL)
        return NDL_NULL_INDEX;

    return ndl_slab_scan(slab, 0);
}

ndl_slab_index ndl_slab_next(ndl_slab *slab, ndl_slab_index last) {

    if ((slab == NULL) || (last == NDL_NULL_INDEX))
        return NDL_NULL_INDEX;

    if (last >= (slab->block_count * slab->block_size))
        return NDL_NULL_INDEX;

    return ndl_slab_scan(slab, last + 1);
}


uint64_t ndl_slab_cap(ndl_slab *slab) {

    if (slab == NULL)
        return 0;

    return slab->block_count * slab->block_size;
}

uint64_t ndl_slab_size(ndl_slab *slab) {

    if (slab == NULL)
        return 0;

    return slab->elem_count;
}

void ndl_slab_print(ndl_slab *slab) {

    printf("Printing slab.\n");
    printf("Elem, block size: %ld, %ld.\n", slab->elem_size, slab->block_size);
    printf("Block count, cap: %ld, %ld.\n", slab->block_count, slab->block_cap);
    printf("Elem count: %ld.\n", slab->elem_count);
    printf("Free list head: %ld.\n", slab->free_head);

    uint64_t i;
    for (i = 0; i < slab->block_count; i++) {

        printf("Block %ld:\n", i);

        uint8_t *block = (uint8_t *) slab->blocks[i];
        ndl_slab_item *item;

        uint64_t index;
        for (index = 0; index < slab->block_size; index++) {

            item = (ndl_slab_item *) (block + index * (slab->elem_size + sizeof(ndl_slab_item)));
            printf("[next: %04ld]\n", item->next_free);
        }
    }

    return;
}
