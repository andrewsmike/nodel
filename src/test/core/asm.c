#include "test.h"

#include "asm.h"

char *ndl_test_asm_syntax(void) {

    char *src1 =
        "loop:   \t          \r\n"
        "\tloop2:   #Labels.   \n"
        "add a, b\t-> c #WOOOO \n"
        "sub l,q->n | l=3      \n"
        "add a, 10 -> b        \n"
        "ollo :bleh | ab=bc    \n"
        "ollo :bleh |a=:bleh d=4.2 \n"
        "add a, 10.3 -> b      \n"
        "bleh:                 \n"
        "add b, -10 -> b       \n"
        "add b, -10.3 -> b     \n"
        "load :bleh, syma -> b \n";

    ndl_asm_result res = ndl_asm_parse(src1, NULL);
    if (res.msg != NULL) {
        ndl_asm_print_err(res);
        return "Failed to assemble program";
    }

    ndl_graph_kill(res.graph);

    return 0;
}
