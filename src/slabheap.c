#include "slabheap.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <stdio.h>

ndl_slabheap *ndl_slabheap_init(uint64_t data_size, ndl_slabheap_cmp_func compare,
                                uint64_t slab_block_size) {

    void *region = malloc(ndl_slabheap_msize(data_size, compare, slab_block_size));
    if (region == NULL)
        return NULL;

    ndl_slabheap *ret = ndl_slabheap_minit(region, data_size, compare, slab_block_size);
    if (ret == NULL)
        free(region);

    return ret;
}

void ndl_slabheap_kill(ndl_slabheap *heap) {

    if (heap == NULL)
        return;

    ndl_slabheap_mkill(heap);

    free(heap);
}

ndl_slabheap *ndl_slabheap_minit(void *region, uint64_t data_size,
                                 ndl_slabheap_cmp_func compare, uint64_t slab_block_size) {

    ndl_slabheap *heap = (ndl_slabheap *) region;
    if (heap == NULL)
        return NULL;

    ndl_slab *ret = ndl_slab_minit((ndl_slab *) &heap->slab,
                                   sizeof(ndl_slabheap_node) + data_size,
                                   slab_block_size);
    if (ret == NULL)
        return NULL;

    heap->data_size = data_size;
    heap->compare = compare;
    heap->root = heap->foot = NULL;

    return heap;
}

void ndl_slabheap_mkill(ndl_slabheap *heap) {

    if (heap == NULL)
        return;

    ndl_slab_mkill((ndl_slab *) &heap->slab);

    return;
}

uint64_t ndl_slabheap_msize(uint64_t data_size, ndl_slabheap_cmp_func compare,
                            uint64_t slab_block_size) {

    return sizeof(ndl_slabheap)
        + ndl_slab_msize(data_size + sizeof(ndl_slabheap_node), slab_block_size);
}

void *ndl_slabheap_head(ndl_slabheap *heap) {

    if (heap->root == NULL)
        return NULL;

    void *data = &heap->root->data;

    int err = ndl_slabheap_del(heap, heap->root);
    if (err != 0)
        return NULL;

    return data;
}

void *ndl_slabheap_peek(ndl_slabheap *heap) {

    if (heap->root == NULL)
        return NULL;

    return &heap->root->data;
}

/* The foot pointer points to the next node to be used
 * by the put/del operations, either as the location to
 * initially push or as the node to replace the newly deleted
 * node.
 * This algorithm keeps the heap completely balance, and refers to
 * the first node in the second to last (or last, if perfectly filled)
 * layer where there are one or more available places to put children nodes.
 *
 * Foot can be NULL if the tree is empty.
 *
 * next: run until you encounter a node with a NULL child.
 * Work your way up the tree while you're coming from the right node.
 * When you hit the root, or you come from the left node, recurse to
 * the leftmost node from the right child.
 */
static ndl_slabheap_node *ndl_slabheap_footnext(ndl_slabheap *heap, ndl_slabheap_node *start) {

    if (start == NULL)
        return NULL;

    /* If either children are NULL, return. */
    if ((start->left == NULL) || (start->right == NULL))
        return start;

    /* Ascend until we are coming from the left, or we hit the top. */
    ndl_slabheap_node *last = start;
    ndl_slabheap_node *next = start;
    while (next != NULL) {

        if (next->left == last) {
            next = next->right;
            break;
        }

        last = next;
        next = next->parent;
    }

    if (next == NULL)
        next = last;

    /* Descend to left most node. */
    while (next != NULL) {
        last = next;
        next = next->left;
    }

    return last;
}

/* If there is at least one child node, stay.
 * Work your way up the tree while you're coming from the left node.
 * When you hit the root, or you come from the right node, recurse to
 * the rightmost node from the left child. Move to the node's parent.
 */
static ndl_slabheap_node *ndl_slabheap_footprev(ndl_slabheap *heap, ndl_slabheap_node *start) {

    if (start == NULL)
        return NULL;

    /* If either children are not NULL, return. */
    if ((start->left != NULL) || (start->right != NULL))
        return start;

    /* Ascend until we are coming from the left, or we find a leaf. */
    ndl_slabheap_node *last = start;
    ndl_slabheap_node *next = start;
    while (next != NULL) {

        if (next->right == last) {
            next = next->left;
            break;
        }

        last = next;
        next = next->parent;
    }

    if (next == NULL)
        next = last;

    /* Descend to left most node. */
    while (next->right != NULL) {
        last = next;
        next = next->right;
    }

    return last;
}

static inline void ndl_slabheap_swap(ndl_slabheap *heap,
                                     ndl_slabheap_node *a, ndl_slabheap_node *b) {

    if (heap->foot == a) heap->foot = b;
    else if (heap->foot == b) heap->foot = a;

    if (heap->root == a) heap->root = b;
    else if (heap->root == b) heap->root = a;

    if (a->parent != NULL) {
        if (a == a->parent->left) a->parent->left  = b;
        else                      a->parent->right = b;
    }


    if (a->left != NULL)  a->left->parent  = b;
    if (a->right != NULL) a->right->parent = b;

    if (b->parent != NULL) {
        if (b == b->parent->left) b->parent->left  = a;
        else                      b->parent->right = a;
    }

    if (b->left != NULL) b->left->parent = a;
    if (b->right != NULL) b->right->parent = a;

    ndl_slabheap_node *aparent, *aleft, *aright;
    aparent = a->parent; a->parent = b->parent; b->parent = aparent;
    aleft   =   a->left; a->left   =   b->left; b->left   = aleft;
    aright  =  a->right; a->right  =  b->right; b->right  = aright;

    return;
}

static inline void ndl_slabheap_bubble(ndl_slabheap *heap, ndl_slabheap_node *node) {

    int cmp;
    while (node->parent != NULL) {
        cmp = heap->compare((void *) &node->data, (void *) &node->parent->data);
        if (cmp < 0) {
            ndl_slabheap_swap(heap, node, node->parent);
            node = node->parent;
        } else {
            return;
        }
    }
}

static inline void ndl_slabheap_sink(ndl_slabheap *heap, ndl_slabheap_node *node) {

    int cmp;
    while (1) {
        if (node->left != NULL) {

            cmp = heap->compare((void *) &node->data, (void *) &node->left->data);
            if (cmp > 0) {
                ndl_slabheap_swap(heap, node, node->left);
                node = node->left;
                continue;
            }
        }
        if (node->right != NULL) {

            cmp = heap->compare((void *) &node->data, (void *) &node->right->data);
            if (cmp > 0) {
                ndl_slabheap_swap(heap, node, node->right);
                node = node->right;
                continue;
            }
        }

        return;
    }
}

static inline void ndl_slabheap_insert(ndl_slabheap *heap, ndl_slabheap_node *node) {

    if (heap->root == NULL) {

        heap->root = node;
        heap->foot = node;
    }

    if (heap->foot->left == NULL) {
        heap->foot->left = node;
        node->parent = heap->foot->left;
    } else if (heap->foot->right == NULL) {
        heap->foot->right = node;
        node->parent = heap->foot->right;
        heap->foot = ndl_slabheap_footnext(heap, heap->foot);
    }

    ndl_slabheap_bubble(heap, node);
}

static inline void ndl_slabheap_delete(ndl_slabheap *heap, ndl_slabheap_node *node) {

    /* Uhhh.... */
    if (heap->root == NULL)
        return;

    ndl_slabheap_node *from;
    if (heap->foot->left != NULL) {

        from = heap->foot->left;
    } else {

        heap->foot = ndl_slabheap_footprev(heap, heap->foot);

        if (heap->foot->right != NULL)
            from = heap->foot->right;
        else if (heap->foot->left != NULL)
            from = heap->foot->left;
        else
            from = NULL;
    }

    if (from != NULL) {

        ndl_slabheap_swap(heap, from, node);
        if (from->parent->left == from)
            from->parent->left = NULL;
        else
            from->parent->right = NULL;

    } else {

        heap->root = heap->foot = NULL;
    }

    node->parent = NULL;

    /* You can delete from anywhere, so we may have to bubble too. */
    ndl_slabheap_sink(heap, node);
    ndl_slabheap_bubble(heap, node);
}

void *ndl_slabheap_node_head(ndl_slabheap *heap) {

    ndl_slab_index first = ndl_slab_head((ndl_slab *) &heap->slab);
    if (first == NDL_NULL_INDEX)
        return NULL;

    ndl_slabheap_node *ret = (ndl_slabheap_node *)
        ndl_slab_get((ndl_slab *) &heap->slab, first);
    if (ret == NULL)
        return NULL;

    ndl_slabheap_delete(heap, ret);

    return &ret->data;
}

void *ndl_slabheap_node_next(ndl_slabheap *heap, void *prev) {

    if (prev == NULL)
        return NULL;

    uint8_t *data = ((uint8_t *) prev) - offsetof(ndl_slabheap_node, data);
    ndl_slab_index index = ((ndl_slabheap_node *) data)->sid;

    ndl_slab_index next = ndl_slab_next((ndl_slab *) &heap->slab, index);
    if (next == NDL_NULL_INDEX)
        return NULL;

    ndl_slabheap_node *ret = (ndl_slabheap_node *)
        ndl_slab_get((ndl_slab *) &heap->slab, next);
    if (ret == NULL)
        return NULL;

    return &ret->data;
}

void ndl_slabheap_readj(ndl_slabheap *heap, void *data) {

    uint8_t *nodedata = ((uint8_t *) data) - offsetof(ndl_slabheap_node, data);
    ndl_slabheap_node *node = (ndl_slabheap_node *) nodedata;

    if (node == NULL)
        return;

    ndl_slabheap_bubble(heap, node);
    ndl_slabheap_sink(heap, node);
}

void *ndl_slabheap_put(ndl_slabheap *heap, void *data) {
    ndl_slab_index index = ndl_slab_alloc((ndl_slab *) &heap->slab);
    if (index == NDL_NULL_INDEX)
        return NULL;

    ndl_slabheap_node *node = ndl_slab_get((ndl_slab *) &heap->slab, index);
    if (node == NULL) {
        ndl_slab_free((ndl_slab *) &heap->slab, index);
        return NULL;
    }

    node->sid = index;
    node->parent = NULL;
    node->left = node->right = NULL;
    memcpy((void *) &node->data, data, heap->data_size);

    ndl_slabheap_insert(heap, node);

    return node;
}

int ndl_slabheap_del(ndl_slabheap *heap, void *data) {

    uint8_t *nodedata = ((uint8_t *) data) - offsetof(ndl_slabheap_node, data);
    ndl_slabheap_node *node = (ndl_slabheap_node *) nodedata;

    if (node == NULL)
        return -1;

    ndl_slabheap_delete(heap, node);

    ndl_slab_free((ndl_slab *) &heap->slab, node->sid);

    return 0;
}

uint64_t ndl_slabheap_data_size(ndl_slabheap *heap) {

    return heap->data_size;
}

ndl_slabheap_cmp_func ndl_slabheap_compare(ndl_slabheap *heap) {

    return heap->compare;
}

uint64_t ndl_slabheap_size(ndl_slabheap *heap) {

    return ndl_slab_size((ndl_slab *) &heap->slab);
}

uint64_t ndl_slabheap_cap(ndl_slabheap *heap) {

    return ndl_slab_cap((ndl_slab *) &heap->slab);
}

void ndl_slabheap_print(ndl_slabheap *heap) {

    printf("Printing heap.\n");
    printf("Data size, compare: %ld, %p.\n", heap->data_size, (void *) &heap->compare);
    ndl_slab_print((ndl_slab *) &heap->slab);
}
