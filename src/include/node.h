#ifndef NODEL_NODE_H
#define NODEL_NODE_H

#include <stdint.h>

typedef enum nodel_value_type_e {

    ENONE  = -1,   // Unknown
    EREF   =  0,   // Reference to other node.
    ESYM   =  1,   // 8 char long string. Padded with spaces, no NULL terminator.
    EINT   =  2,   // Integer value.
    EFLOAT =  3,   // Floating point value.

} nodel_value_type;


typedef int32_t nodel_ref;
#define NODEL_NULL ((nodel_ref) -1)

typedef uint64_t nodel_sym;
#define NODEL_NULL_SYM ((nodel_sym) 0)
typedef int64_t nodel_int;
typedef double nodel_float;


typedef struct nodel_value_s {

    nodel_value_type type;
    union {
        nodel_ref ref; // EREF
        nodel_sym sym; // ESYM
        nodel_int num; // EINT
        nodel_float real; // EREAL
    };

} nodel_value;

typedef struct nodel_node_pool_s nodel_node_pool;

nodel_node_pool *nodel_node_pool_init(void);
void             nodel_node_pool_kill(nodel_node_pool *pool);

nodel_ref nodel_node_pool_alloc(nodel_node_pool *pool);

int nodel_node_pool_set(nodel_node_pool *pool, nodel_ref node, nodel_sym key, nodel_value val);
nodel_value nodel_node_pool_get(nodel_node_pool *pool, nodel_ref node, nodel_sym key);

int nodel_node_pool_get_size(nodel_node_pool *pool, nodel_ref node);
nodel_sym nodel_node_pool_get_key(nodel_node_pool *pool, nodel_ref node, int index);

#endif /* NODEL_NODE_H */
