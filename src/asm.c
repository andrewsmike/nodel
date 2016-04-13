#include "asm.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "node.h"
#include "graph.h"

/* Basic assembler.
 * Parse a string into the intermediate assembly format and build
 * the desired function into an ndl_graph.
 * As it turns out, it's pretty easy to generate the graph directly
 * from a stateful process as you parse the string. The intermediate
 * format is primarily made of a graph that is eventually copied on
 * generation.
 */

struct ndl_asm_script_s {

    const char *source;

    ndl_ref root, label_node, curr_inst;

    ndl_graph *graph;
};

/* Initialize the graph datastructure for generation.
 * Has a root node, a label mapping node, and the first instruction.
 * Root has pointers to the label, the current, and the head(same as current.)
 */
static int ndl_asm_parse_init(ndl_asm_script *script) {

    ndl_ref root = ndl_graph_alloc(script->graph);
    if (root == NDL_NULL_REF)
        return -1;

    ndl_ref labels = ndl_graph_salloc(script->graph, root, NDL_SYM("labels  "));
    if (labels == NDL_NULL_REF)
        return -1;

    ndl_ref head = ndl_graph_salloc(script->graph, root, NDL_SYM("head    "));
    if (head == NDL_NULL_REF)
        return -1;

    int err = ndl_graph_set(script->graph, root, NDL_SYM("curr    "), NDL_VALUE(EVAL_REF, ref=head));
    if (err != 0)
        return -1;

    script->root = root;
    script->label_node = labels;
    script->curr_inst = head;

    return 0;
}

/* Useful macros. Will probably replace IS_TOKEN() with individual test macros. */
#define NDL_DEF_TOKEN_CHECK(name, test)                   \
    static inline int ndl_asm_parse_is_ ## name(char c) { \
        if (test)                                         \
            return 1;                                     \
        else                                              \
            return 0;                                     \
    }
#define IS_TOKEN(name, src) (ndl_asm_parse_is_ ##name(src[0]))

NDL_DEF_TOKEN_CHECK(symbol, (((c >= 'a') && (c <= 'z')) ||
                             ((c >= 'A') && (c <= 'Z')) ||
                             ((c == '_') || (c == '_'))))
NDL_DEF_TOKEN_CHECK(isymbol, (((c >= 'a') && (c <= 'z')) ||
                              ((c >= 'A') && (c <= 'Z')) ||
                              ((c >= '0') && (c <= '9')) ||
                              ((c == '_') || (c == '_'))))
NDL_DEF_TOKEN_CHECK(label, c == ':')
NDL_DEF_TOKEN_CHECK(num, ((c >= '0') && (c <= '9')) || (c == '-'))
NDL_DEF_TOKEN_CHECK(numins, (c >= '0') && (c <= '9'))
NDL_DEF_TOKEN_CHECK(numsep, c == '.')
NDL_DEF_TOKEN_CHECK(obj, (IS_TOKEN(symbol, (&c)) ||
                          IS_TOKEN(label, (&c)) ||
                          IS_TOKEN(num, (&c))))

NDL_DEF_TOKEN_CHECK(ws, c == ' ' || c == '\t')
NDL_DEF_TOKEN_CHECK(comment, c == '#')
NDL_DEF_TOKEN_CHECK(bar, c == '|')
NDL_DEF_TOKEN_CHECK(eol, c == '\n' || c == '\r')
NDL_DEF_TOKEN_CHECK(eos, c == '\0')
NDL_DEF_TOKEN_CHECK(sep, c == '-' || c == ',' || c == '.')

/* Various primitives. */
static inline long int ndl_asm_parse_eat_ws(const char *src) {

    const char *curr = src;

    while (IS_TOKEN(ws, curr))
           curr++;

    return curr - src;
}

static inline long int ndl_asm_parse_eat_comment(const char *src) {

    const char *curr = src;
    while (!IS_TOKEN(eol, curr) && !IS_TOKEN(eos, curr))
        curr++;

    if (IS_TOKEN(eos, curr))
        return curr - src;
    else
        return curr - src + 1;
}

static inline long int ndl_asm_parse_eat_sym(const char *src, ndl_sym *ret) {

    const char *curr = src;
    while (IS_TOKEN(isymbol, curr)) curr++;

    long int size = curr - src;
    if (size > 8 || size == 0)
        return -1;

    if (ret == NULL)
        return size;

    long int i;
    for (i = 0; i < 8; i++)
        ((char *) ret)[i] = (i < size)? src[i] : ' ';

    return size;
}

static inline long int ndl_asm_parse_eat_label(const char *src, ndl_sym *ret) {

    if (!IS_TOKEN(label, src))
        return -1;

    long int off = ndl_asm_parse_eat_sym(src + 1, ret);
    if (off < 0)
        return -1;

    return off + 1;
}

static inline long int ndl_asm_parse_marker(const char *src, ndl_asm_script *env) {

    ndl_sym sym;
    long int off = ndl_asm_parse_eat_sym(src, &sym);
    if (off < 0)
        return -1;

    int err = ndl_graph_set(env->graph, env->label_node, sym, NDL_VALUE(EVAL_REF, ref=env->curr_inst));
    if (err != 0)
        return -1;

    const char *curr = src + off + 1;

    while (IS_TOKEN(ws, curr))
        curr++;

    if (IS_TOKEN(comment, curr))
        return (curr - src) + ndl_asm_parse_eat_comment(curr);
    else if (IS_TOKEN(eol, curr))
        return (curr - src) + 1;
    else if (IS_TOKEN(eos, curr))
        return (curr - src);
    else
        return -1;
}

static long int ndl_asm_parse_num(const char *src, ndl_asm_script *env, ndl_sym argname) {

    int inv = 0;
    if (src[0] == '-') {
        inv = 1;
        src++;
    }

    int64_t ival = 0;
    const char *search = src;
    while (IS_TOKEN(numins, search)) {
        ival *= 10;
        ival += (search[0] - '0');
        search++;
    }

    ndl_value val;
    if (!IS_TOKEN(numsep, search)) {
        val.type = EVAL_INT;
        val.num = ival;
    } else if (IS_TOKEN(numsep, search)) {
        val.type = EVAL_FLOAT;
        val.real = (double) ival;
        double scale = 0.1;

        if (!IS_TOKEN(numins, (search+1)))
            return -1;

        search++;
        while (IS_TOKEN(numins, search)) {
            val.real += scale * (search[0] - '0');
            scale *= 0.1;
            search++;
        }
    }

    if (inv) {
        if (val.type == EVAL_FLOAT)
            val.real = - val.real;
        else
            val.num = - val.num;
    }

    int err = ndl_graph_set(env->graph, env->curr_inst, argname, val);
    if (err != 0)
        return -1;

    if (inv)
        return search - src + 1;
    else
        return search - src;
}

/* TODO: Resolve references at end of parsing. */
static long int ndl_asm_parse_label(const char *src, ndl_asm_script *env, ndl_sym argname) {

    ndl_sym ret;
    long int off = ndl_asm_parse_eat_label(src, &ret);
    if (off < 0)
        return -1;

    ndl_value to = ndl_graph_get(env->graph, env->label_node, ret);
    if (to.type != EVAL_REF || to.ref == NDL_NULL_REF)
        return -1;

    int err = ndl_graph_set(env->graph, env->curr_inst, argname, to);
    if (err != 0)
        return -1;

    return off;
}

static long int ndl_asm_parse_sym(const char *src, ndl_asm_script *env, ndl_sym argname) {

    ndl_sym ret;
    long int off = ndl_asm_parse_eat_sym(src, &ret);
    if (off < 0)
        return -1;

    int err = ndl_graph_set(env->graph, env->curr_inst, argname, NDL_VALUE(EVAL_SYM, sym=ret));
    if (err != 0)
        return -1;

    return off;
}

static long int ndl_asm_parse_obj(const char *src, ndl_asm_script *env, ndl_sym argname) {

    if (IS_TOKEN(num, src))
        return ndl_asm_parse_num(src, env, argname);
    if (IS_TOKEN(label, src))
        return ndl_asm_parse_label(src, env, argname);
    if (IS_TOKEN(symbol, src))
        return ndl_asm_parse_sym(src, env, argname);
    else
        return -1;
}

static long int ndl_asm_parse_arglist(const char *src, ndl_asm_script *env) {

    char symname[8];
    memcpy(symname, "sym     ", 8);
    char *sym = symname;

    const char *curr = src;

    /* headchew */
    while (IS_TOKEN(ws, curr))
        curr++;

    if (IS_TOKEN(eol, curr))
        return (curr - src) + 1;
    else if (IS_TOKEN(eos, curr))
        return (curr - src);

    /* head */
    if (!IS_TOKEN(obj, curr))
        return (curr - src);

    symname[3] = 'a';
    long int off = ndl_asm_parse_obj(curr, env, NDL_SYM(sym));
    if (off < 0)
        return -1;

    curr += off;

    long int it;
    for (it = 1; it < 26; it++) {

        /* sargchew */
        while (IS_TOKEN(ws, curr))
            curr++;

        /* sarg */
        if (!IS_TOKEN(sep, curr))
            return curr - src;

        if (curr[0] == '-') {
            if (curr[1] != '>')
                return -1;
            curr += 2;
        } else {
            curr += 1;
        }

        /* argchew */
        while (IS_TOKEN(ws, curr))
            curr++;

        /* arg */
        if (!IS_TOKEN(obj, curr))
            return -1;

        symname[3] = (char) ('a' + it);
        off = ndl_asm_parse_obj(curr, env, NDL_SYM(sym));
        if (off < 0)
            return -1;

        curr += off;
    }

    return -1;
}

static long int ndl_parse_kvlist(const char *src, ndl_asm_script *env) {

    /* TODO: kv lists. */
    return 0;

}

/* Parse a line starting with a symbol.
 * If we're dealing with a label, rewind, return parse_marker().
 * Else, we're parsing a typical opcode.
 * parse the opcode, save to node.
 * parse the symbol list.
 * parse the kv pairs.
 * - Comment: eat, return.
 * - EOL: eat, return.
 * - EOS: return.
 * - Error
 */
static long int ndl_asm_parse_nline(const char *src, ndl_asm_script *env) {

    ndl_sym opcode;
    const char *curr = src;
    long int off = ndl_asm_parse_eat_sym(curr, &opcode);
    if (off < 0)
        return -1;

    curr += off;

    if (IS_TOKEN(label, curr))
        return ndl_asm_parse_marker(src, env);

    /* Parsing a regular opcode. */
    int err = ndl_graph_set(env->graph, env->curr_inst, NDL_SYM("opcode  "), NDL_VALUE(EVAL_SYM, sym=opcode));
    if (err != 0)
        return -1;

    off = ndl_asm_parse_arglist(curr, env);
    if (off < 0)
        return -1;

    curr += off;

    /* We create a new node each time. */
    ndl_ref next = ndl_graph_salloc(env->graph, env->curr_inst, NDL_SYM("next    "));
    if (next == NDL_NULL_REF)
        return -1;

    /* Parse the KV list after salloc, overwrite self.next. */
    off = ndl_parse_kvlist(curr, env);
    if (off < 0)
        return -1;

    curr += off;

    /* Update the instruction pointer. */
    env->curr_inst = next;

    if (IS_TOKEN(comment, curr)) {
        return (curr - src) + ndl_asm_parse_eat_comment(curr);
    } else if (IS_TOKEN(eol, curr)) {
        return (curr - src) + 1;
    } else if (IS_TOKEN(eos, curr)) {
        return (curr - src);
    } else {
        return -1;
    }
}

/* Switch src[0]:
 * - Whitespace: Eat whitespace, recurse.
 * - Comment: Eat comment, return.
 * - Symbol: Eat label|opcode, return.
 * - EOL: eat, return.
 * - Error
 * Eats the newline, if there is one.
 */
static long int ndl_asm_parse_line(const char *src, ndl_asm_script *env) {

    long int off;
    if (IS_TOKEN(ws, src)) {

        off = ndl_asm_parse_eat_ws(src);

        return ndl_asm_parse_line(src + off, env) + off;

    } else if (IS_TOKEN(comment, src)) {

        return ndl_asm_parse_eat_comment(src);

    } else if (IS_TOKEN(eol, src)) {

        return 1;

    } else if (IS_TOKEN(eos, src)) {

        return 0;

    } else if (IS_TOKEN(symbol, src)) {

        return ndl_asm_parse_nline(src, env);

    } else {

        return -1;
    }
}

ndl_asm_script *ndl_asm_parse(const char *source) {

    ndl_asm_script *env = malloc(sizeof(ndl_asm_script));
    if (env == NULL)
        return NULL;

    env->graph = ndl_graph_init();
    if (env->graph == NULL) {
        free(env);
        return NULL;
    }

    /* Initialize the  graph datastructure. */
    if (ndl_asm_parse_init(env) != 0) {
        free(env);
        ndl_graph_kill(env->graph);
        return NULL;
    }

    env->source = source;

    while (*source != '\0') {
        long int res = ndl_asm_parse_line(source, env);
        if (res <= 0) {
            free(env);
            ndl_graph_kill(env->graph);
            return NULL;
        }
        source += res;
    }

    return env;
}

void ndl_asm_kill(ndl_asm_script *script) {

    if (script == NULL)
        return;

    if (script->graph != NULL)
        ndl_graph_kill(script->graph);

    free(script);
}

void ndl_asm_print(ndl_asm_script *script) {

    printf("Printing assembly.\n");
    printf("Source:\n--------\n%s\n--------\n", script->source);
    printf("Root, label nodes: %d, %d.\n", script->root, script->label_node);

    ndl_graph_print(script->graph);
}
