#ifndef NODEL_NODE_H
#define NODEL_NODE_H

#include <stdint.h>


/* Values hold either data or references to nodes.
 * Supported datatypes are ints, floats, and symbols.
 * References can be NULL. Nodes are capped at 2Bn.
 *
 * You can perform boolean operations on integers.
 * You can perform basic math operations on integers and floats.
 *
 * Symbols are unsigned 64 bit ints. They can be converted to/from integers.
 * Typically, symbols will represent a string like so: *((uint64_t*)"somesym ").
 * Strings will be lower case, 8 long, space padded, unterminated.
 * Endian is left to the platform.
 *
 * Functions that return a nodel value may use type=ENONE as an error indicator.
 */


enum ndl_value_type_e {

    EVAL_NONE,  /* Invalid value. */

    EVAL_REF,   /* Reference to a node. */
    EVAL_SYM,   /* Symbol. */
    EVAL_INT,   /* Integer number. */
    EVAL_FLOAT, /* Floating point number. */

    EVAL_SIZE

};

/* Get a pretty-print unpadded names for each type. Max strlen() = 5. */
extern const char *ndl_value_type_to_string[EVAL_SIZE];


/* C datatypes for each value datatype. */
typedef  int32_t ndl_ref;
typedef uint64_t ndl_sym;
typedef  int64_t ndl_int;
typedef double   ndl_float;

/* NULL / invalid values for some datatypes and other macros. */
#define NDL_NULL_REF ((ndl_ref) -1)
#define NDL_NULL_SYM ((ndl_sym)  0)

#define NDL_SYM(sym) *((uint64_t*) sym)


/* Actual value structure. */
struct ndl_value_s {

    enum ndl_value_type_e type;
    union {
        ndl_ref   ref;  /* EVAL_REF   */
        ndl_sym   sym;  /* EVAL_SYM   */
        ndl_int   num;  /* EVAL_INT   */
        ndl_float real; /* EVAL_FLOAT */
    };
};
typedef struct ndl_value_s ndl_value;

/* Pretty prints a value.
 * NULL terminates, returns strlen(buff).
 */
int ndl_value_to_string(ndl_value value, int len, char *buff);

/* Value construction macro.
 * Usage: NDL_VALUE(EVAL_SYM, sym=NDL_SYM("hello   "))
 *        NDL_VALUE(EVAL_INT, num=27)
 */
#define NDL_VALUE(type_e, value) ((ndl_value) {.type = type_e, .value})


#endif /* NODEL_NODE_H */
