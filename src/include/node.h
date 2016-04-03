#ifndef NODEL_NODE_H
#define NODEL_NODE_H

#include <stdint.h>

typedef enum ndl_value_type_e {

    ENONE  = -1,   // Unknown
    EREF   =  0,   // Reference to other node.
    ESYM   =  1,   // 8 char long string. Padded with spaces, no NULL terminator.
    EINT   =  2,   // Integer value.
    EFLOAT =  3,   // Floating point value.

} ndl_value_type;


typedef int32_t ndl_ref;
#define NDL_NULL ((ndl_ref) -1)

typedef uint64_t ndl_sym;
#define NDL_NULL_SYM ((ndl_sym) 0)
typedef int64_t ndl_int;
typedef double ndl_float;


typedef struct ndl_value_s {

    ndl_value_type type;
    union {
        ndl_ref ref; // EREF
        ndl_sym sym; // ESYM
        ndl_int num; // EINT
        ndl_float real; // EREAL
    };

} ndl_value;

#endif /* NODEL_NODE_H */
