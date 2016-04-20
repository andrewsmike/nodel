#include "test.h"

#include "node.h"

#include <string.h>

typedef struct ndl_value_expected_s {

    ndl_value value;
    const char *expected;

} ndl_value_expected;

/* TODO: Long ints and floats are not printed correctly.
 * Correct this.
 */

char *ndl_test_node_value_print(void) {

    ndl_value values[] = {
        NDL_VALUE(EVAL_NONE, ref=27),
        NDL_VALUE(EVAL_REF, ref=NDL_NULL_REF),
        NDL_VALUE(EVAL_REF, ref=0x0001),
        NDL_VALUE(EVAL_REF, ref=0xABC4),
        NDL_VALUE(EVAL_REF, ref=0xABCDEF),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("hello   ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("\0hello ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("\0\0\0\0\0\0\0\0")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("        ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("next    ")),
        NDL_VALUE(EVAL_SYM, sym=NDL_SYM("    last")),
        NDL_VALUE(EVAL_INT, num=0),
        NDL_VALUE(EVAL_INT, num=-1),
        NDL_VALUE(EVAL_INT, num=1),
        NDL_VALUE(EVAL_INT, num=10000),
        NDL_VALUE(EVAL_INT, num=10000009),
        NDL_VALUE(EVAL_INT, num=-10000000009),
        NDL_VALUE(EVAL_FLOAT, real=-100000000.0),
        NDL_VALUE(EVAL_FLOAT, real=100000000.0),
        NDL_VALUE(EVAL_FLOAT, real=1000.03),
        NDL_VALUE(EVAL_FLOAT, real=0.00000000004),
        NDL_VALUE(EVAL_FLOAT, real=-0.00000000004),
        NDL_VALUE(EVAL_FLOAT, real=-0.00434),
        NDL_VALUE(EVAL_FLOAT, real=1.3287),
        NDL_VALUE(EVAL_NONE, ref=NDL_NULL_REF)
    };

    const char *expected[] = {
        "[None:        ]",
        "[Ref:     null]",
        "[Ref:        1]",
        "[Ref:     ABC4]",
        "[Ref:   ABCDEF]",
        "[Sym: hello   ]",
        "[Sym: 0hello 0]",
        "[Sym: 00000000]",
        "[Sym:         ]",
        "[Sym: next    ]",
        "[Sym:     last]",
        "[Int:        0]",
        "[Int: -      1]",
        "[Int:        1]",
        "[Int:    10000]",
        "[Int: 10000009]",
        "[Int: -#######]",
        "[Float: -10000]",
        "[Float:1000000]",
        "[Float:1000.03]",
        "[Float:#######]",
        "[Float: -#####]",
        "[Float: -0.004]",
        "[Float:1.32870]"
    };

    char buff[16];
    buff[15] = '\0';

    int i;
    for (i = 0; values[i].type != EVAL_NONE || values[i].ref != NDL_NULL_REF; i++) {
        ndl_value_to_string(values[i], 15, buff);
        if (strcmp(expected[i], buff) != 0) {
            printf("Expected: %s.\n", expected[i]);
            printf("Received: %s.\n", buff);
            return "Pretty print returned different result";
        }
    }

    return 0;
}

