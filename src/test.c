/** nodel/src/test.c: Test implemented functionality. */

#include <stdio.h>
#include <stdlib.h>

#include "node.h"
#include "graph.h"

int testprettyprint(void) {

    printf("Testing value pretty printing.\n");

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

    char buff[17];
    buff[16] = '\0';

    int i;
    for (i = 0; values[i].type != EVAL_NONE || values[i].ref != NDL_NULL_REF; i++) {
        ndl_value_to_string(values[i], 16, buff);
        printf("%2ith value: %s.\n", i, buff);
    }

    return 0;
}

int main(int argc, const char *argv[]) {

    printf("Beginning tests.\n");

    int err = testprettyprint();

    printf("Finished tests.\n");

    return err;
}
