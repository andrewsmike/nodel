#include "asm.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "node.h"
#include "graph.h"


/* Token comparison macros. */
#define IS_TOKEN_SYMBOL(c) (((c >= 'a') && (c <= 'z')) ||       \
                            ((c >= 'A') && (c <= 'Z')) ||       \
                            ((c == '_')))
#define IS_TOKEN_ISYMBOL(c) (((c >= 'a') && (c <= 'z')) ||      \
                             ((c >= 'A') && (c <= 'Z')) ||      \
                             ((c >= '0') && (c <= '9')) ||      \
                             ((c == '_')))
#define IS_TOKEN_NUM(c) (((c >= '0') && (c <= '9')) || (c == '-'))
#define IS_TOKEN_INUM(c) (((c >= '0') && (c <= '9')))

#define IS_TOKEN_OBJ(c) (IS_TOKEN_SYMBOL(c) || \
                         IS_TOKEN_LABEL(c) || \
                         IS_TOKEN_NUM(c))

#define IS_TOKEN_WS(c) ((c == ' ') || (c == '\t'))
#define IS_TOKEN_EOL(c) ((c == '\n') || (c == '\r'))
#define IS_TOKEN_SEP(c) ((c == '-') || (c == ',') || (c == '.'))

#define IS_TOKEN_LABEL(c) (c == ':')
#define IS_TOKEN_NUMSEP(c) (c == '.')
#define IS_TOKEN_COMMENT(c) (c == '#')
#define IS_TOKEN_BAR(c) (c == '|')
#define IS_TOKEN_EQ(c) (c == '=')
#define IS_TOKEN_NEG(c) (c == '-')
#define IS_TOKEN_EOS(c) (c == '\0')


/* Shorthand for error handling. */
#define PARSEFAIL(message)  \
    do {                    \
        res->msg = message; \
        return -1;          \
    } while (0)


/* Initialize the in-graph datastructure and result.
 * In graph:
 * root:
 * - curr:   inst
 * - head:   inst
 * - labels: reftable
 * Current is the head instruction, curr is the tail,
 * and labels are the symbol->instruction mapping for labels.
 */
static int ndl_asm_parse_init(ndl_asm_result *res) {

    res->root = NDL_NULL_REF;
    res->label_table = NDL_NULL_REF;
    res->badref_head = NDL_NULL_REF;
    res->inst_head = NDL_NULL_REF;
    res->inst_tail = NDL_NULL_REF;

    res->msg = NULL;

    ndl_ref root = ndl_graph_alloc(res->graph);
    if (root == NDL_NULL_REF)
        PARSEFAIL("Failed to allocate root node: internal error.\n");

    ndl_ref labels = ndl_graph_salloc(res->graph, root, NDL_SYM("labels  "));
    if (labels == NDL_NULL_REF)
        PARSEFAIL("Failed to allocate label node: internal error.\n");

    ndl_ref head = ndl_graph_salloc(res->graph, root, NDL_SYM("head    "));
    if (head == NDL_NULL_REF)
        PARSEFAIL("Failed to allocate first instruction node: internal error.\n");

    int err = ndl_graph_set(res->graph, root, NDL_SYM("curr    "), NDL_VALUE(EVAL_REF, ref=head));
    if (err != 0)
        PARSEFAIL("Failed to store current instruction reference: internal error.\n");

    res->root = root;
    res->label_table = labels;
    res->inst_head = head;
    res->inst_tail = head;

    return 0;
}

/* Attempts to fail gracefully.
 * Unmarks assembler root node as root,
 * attempts to run GC to clean up nodes.
 */
static void ndl_asm_parse_kill(ndl_asm_result *res) {

    int err = ndl_graph_unmark(res->graph, res->root);

    if (err == 0)
        ndl_graph_clean(res->graph);
}


/* Various parse primitives. */
static inline long int ndl_asm_parse_eat_ws(const char *src, ndl_asm_result *res) {

    const char *curr = src;

    while (IS_TOKEN_WS(curr[0]))
           curr++;

    res->column += curr - src;

    return curr - src;
}

static inline long int ndl_asm_parse_eat_comment(const char *src, ndl_asm_result *res) {

    const char *curr = src;
    while (!IS_TOKEN_EOL(curr[0]) && !IS_TOKEN_EOS(curr[0]))
        curr++;

    if (IS_TOKEN_EOS(curr[0]))
        return curr - src;
    else
        return curr - src + 1;
}

static inline long int ndl_asm_parse_eat_sym(const char *src, ndl_sym *ret, ndl_asm_result *res) {

    const char *curr = src;
    while (IS_TOKEN_ISYMBOL(curr[0])) curr++;

    res->column += curr - src;

    long int size = curr - src;
    if (size > 8)
        PARSEFAIL("Symbols must be eight characters or fewer.");

    if (size == 0)
        PARSEFAIL("Expected symbol.");

    if (ret == NULL)
        return size;

    long int i;
    for (i = 0; i < 8; i++)
        ((char *) ret)[i] = (i < size)? src[i] : ' ';

    return size;
}

static inline long int ndl_asm_parse_eat_label(const char *src, ndl_sym *ret, ndl_asm_result *res) {

    long int off = ndl_asm_parse_eat_sym(src + 1, ret, res);
    if (off < 0)
        return -1;

    res->column++;

    return off + 1;
}

/* Object parsing methods. */
static long int ndl_asm_parse_num(const char *src, ndl_asm_result *res, ndl_sym argname) {

    int inv = 0;
    if (IS_TOKEN_NEG(src[0])) {
        inv = 1;
        src++;
        res->column++;
    }

    int64_t ival = 0;
    const char *search = src;
    while (IS_TOKEN_INUM(search[0])) {
        ival *= 10;
        ival += (search[0] - '0');
        search++;
        res->column++;
    }

    ndl_value val;
    if (!IS_TOKEN_NUMSEP(search[0])) {
        val.type = EVAL_INT;
        val.num = ival;
    } else {
        val.type = EVAL_FLOAT;
        val.real = (double) ival;
        double scale = 0.1;

        if (!IS_TOKEN_INUM(search[1]))
            PARSEFAIL("Expected decimal portion of floating point number.");

        res->column++;
        search++;
        while (IS_TOKEN_INUM(search[0])) {
            val.real += scale * (search[0] - '0');
            scale *= 0.1;
            search++;
            res->column++;
        }
    }

    if (inv) {
        if (val.type == EVAL_FLOAT)
            val.real = - val.real;
        else
            val.num = - val.num;
    }

    int err = ndl_graph_set(res->graph, res->inst_tail, argname, val);
    if (err != 0)
        PARSEFAIL("Failed to store number argument: internal error.");

    if (inv)
        return search - src + 1;
    else
        return search - src;
}

static long int ndl_asm_parse_label(const char *src, ndl_asm_result *res, ndl_sym argname) {

    ndl_sym ret;
    long int off = ndl_asm_parse_eat_label(src, &ret, res);
    if (off < 0)
        return -1;

    /* If already in symbol table, resolve. Else, push to badref list. */
    ndl_value to = ndl_graph_get(res->graph, res->label_table, ret);
    if (to.type == EVAL_REF || to.ref != NDL_NULL_REF) {

        int err = ndl_graph_set(res->graph, res->inst_tail, argname, to);
        if (err != 0)
            PARSEFAIL("Failed to store resolved reference/label: internal error.");
    } else {

        ndl_ref badref = ndl_graph_salloc(res->graph, res->root, NDL_SYM("brefhead"));
        if (badref == NDL_NULL_REF)
            PARSEFAIL("Failed to store delayed label resolution node: internal error.");

        int err = 0;
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("label   "), NDL_VALUE(EVAL_SYM, sym=ret));
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("inst    "), NDL_VALUE(EVAL_REF, ref=res->inst_tail));
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("symbol  "), NDL_VALUE(EVAL_SYM, sym=argname));
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("line    "), NDL_VALUE(EVAL_INT, num=res->line));
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("column  "), NDL_VALUE(EVAL_INT, num=(res->column - off)));
        err |= ndl_graph_set(res->graph, badref, NDL_SYM("brefnext"), NDL_VALUE(EVAL_REF, ref=res->badref_head));
        if (err != 0)
            PARSEFAIL("Failed to store data to delayed label resolution node: internal error.");

        res->badref_head = badref;
    }

    return off;
}

static long int ndl_asm_parse_sym(const char *src, ndl_asm_result *res, ndl_sym argname) {

    ndl_sym ret;
    long int off = ndl_asm_parse_eat_sym(src, &ret, res);
    if (off < 0)
        return -1;

    int err = ndl_graph_set(res->graph, res->inst_tail, argname, NDL_VALUE(EVAL_SYM, sym=ret));
    if (err != 0)
        PARSEFAIL("Failed to store symbol argument: internal error.");

    return off;
}

static long int ndl_asm_parse_obj(const char *src, ndl_asm_result *res, ndl_sym argname) {

    if (IS_TOKEN_NUM(src[0]))
        return ndl_asm_parse_num(src, res, argname);
    if (IS_TOKEN_LABEL(src[0]))
        return ndl_asm_parse_label(src, res, argname);
    if (IS_TOKEN_SYMBOL(src[0]))
        return ndl_asm_parse_sym(src, res, argname);
    else
        PARSEFAIL("Expected number, label, or symbol.");
}

/* Argument and kv argument list parsing. */
/* WS* (ARG (WS* SEP WS* ARG+)*)? */
static long int ndl_asm_parse_arglist(const char *src, ndl_asm_result *res) {

    char symname[8];
    memcpy(symname, "sym     ", 8);
    char *sym = symname;

    const char *curr = src;

    /* Eat whitespace. */
    while (IS_TOKEN_WS(curr[0]))
        curr++;

    res->column += curr - src;

    if (IS_TOKEN_EOL(curr[0]))
        return (curr - src) + 1;
    else if (IS_TOKEN_EOS(curr[0]))
        return (curr - src);

    /* Eat required first object. */
    if (!IS_TOKEN_OBJ(curr[0]))
        return (curr - src);

    symname[3] = 'a';
    long int off = ndl_asm_parse_obj(curr, res, NDL_SYM(sym));
    if (off < 0)
        return -1;

    curr += off;

    long int it;
    for (it = 1; it < 26; it++) {

        const char *t = curr;

        /* Eat whitespace. */
        while (IS_TOKEN_WS(curr[0]))
            curr++;

        res->column += curr - t;

        /* Eat separator. */
        if (!IS_TOKEN_SEP(curr[0]))
            return curr - src;

        if (curr[0] == '-') {
            if (curr[1] != '>')
                PARSEFAIL("Expected separator.");
            curr += 2;
            res->column += 2;
        } else {
            curr += 1;
            res->column += 1;
        }

        t = curr;

        /* Eat whitespace. */
        while (IS_TOKEN_WS(curr[0]))
            curr++;

        res->column += curr - t;

        /* Eat argument. */
        if (!IS_TOKEN_OBJ(curr[0]))
            PARSEFAIL("Expected argument.");

        symname[3] = (char) ('a' + it);
        off = ndl_asm_parse_obj(curr, res, NDL_SYM(sym));
        if (off < 0)
            return -1;

        curr += off;
    }

    PARSEFAIL("Opcodes must have fewer than 26 arguments.");
}

/* WS* SYMBOL WS* '=' WS* OBJECT */
static long int ndl_asm_parse_kvlist_pair(const char *src, ndl_asm_result *res) {

    const char *curr = src;

    /* Chew whitespace. */
    while (IS_TOKEN_WS(curr[0]))
        curr++;

    res->column += curr - src;

    /* Read symbol. */
    if (!IS_TOKEN_SYMBOL(curr[0]))
        return (curr - src);

    ndl_sym name;
    long int off = ndl_asm_parse_eat_sym(curr, &name, res);
    if (off < 0)
        return -1;

    curr += off;

    const char *t = curr;

    /* Chew whitespace. */
    while (IS_TOKEN_WS(curr[0]))
        curr++;

    res->column += curr - t;

    /* Chew '='. */
    if (!IS_TOKEN_EQ(curr[0]))
        PARSEFAIL("Expected '=' in key-value list.");

    curr++;
    res->column++;

    t = curr;

    /* Chew whitespace. */
    while (IS_TOKEN_WS(curr[0]))
        curr++;

    res->column += curr - t;

    /* Chew object. */
    off = ndl_asm_parse_obj(curr, res, name);
    if (off < 0)
        return -1;

    curr += off;

    return curr - src;
}

/* ('|' KVPAIR*)?*/
static long int ndl_asm_parse_kvlist(const char *src, ndl_asm_result *res) {

    if (!IS_TOKEN_BAR(src[0]))
        return 0;

    const char *curr = src + 1;

    res->column++;

    long int off = 1;
    while (off > 0) {

        off = ndl_asm_parse_kvlist_pair(curr, res);
        if (off < 0)
            return -1;

        curr += off;
    }

    return curr - src;
}

/* Line parsing functions. */
static inline long int ndl_asm_parse_marker(const char *src, ndl_asm_result *res) {

    ndl_sym sym;
    long int off = ndl_asm_parse_eat_sym(src, &sym, res);
    if (off < 0)
        return -1;

    int err = ndl_graph_set(res->graph, res->label_table, sym, NDL_VALUE(EVAL_REF, ref=res->inst_tail));
    if (err != 0)
        PARSEFAIL("Failed to store label: internal error.");

    const char *curr = src + off + 1;

    while (IS_TOKEN_WS(curr[0]))
        curr++;

    res->column += curr - src;

    if (IS_TOKEN_COMMENT(curr[0]))
        return (curr - src) + ndl_asm_parse_eat_comment(curr, res);
    else if (IS_TOKEN_EOL(curr[0]))
        return (curr - src) + 1;
    else if (IS_TOKEN_EOS(curr[0]))
        return (curr - src);
    else
        PARSEFAIL("Expected comment, end of line, or end of input.");
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
static long int ndl_asm_parse_nline(const char *src, ndl_asm_result *res) {

    ndl_sym opcode;
    const char *curr = src;
    long int off = ndl_asm_parse_eat_sym(curr, &opcode, res);
    if (off < 0)
        return -1;

    curr += off;

    if (IS_TOKEN_LABEL(curr[0])) {
        res->column -= off;
        return ndl_asm_parse_marker(src, res);
    }

    /* Parsing a regular opcode. */
    int err = ndl_graph_set(res->graph, res->inst_tail, NDL_SYM("opcode  "), NDL_VALUE(EVAL_SYM, sym=opcode));
    if (err != 0)
        PARSEFAIL("Failed to store opcode symbol: internal error.");

    off = ndl_asm_parse_arglist(curr, res);
    if (off < 0)
        return -1;

    curr += off;

    /* We create a new node each time. */
    ndl_ref next = ndl_graph_salloc(res->graph, res->inst_tail, NDL_SYM("next    "));
    if (next == NDL_NULL_REF)
        PARSEFAIL("Failed to create next instruction node: internal error.");

    /* Parse the KV list after salloc, overwrite self.next. */
    off = ndl_asm_parse_kvlist(curr, res);
    if (off < 0)
        return -1;

    curr += off;

    /* Update the instruction pointer. */
    res->inst_tail = next;

    if (IS_TOKEN_COMMENT(curr[0])) {
        return (curr - src) + ndl_asm_parse_eat_comment(curr, res);
    } else if (IS_TOKEN_EOL(curr[0])) {
        return (curr - src) + 1;
    } else if (IS_TOKEN_EOS(curr[0])) {
        return (curr - src);
    } else {
        PARSEFAIL("Expected comment, end of line, or end of input.");
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
static long int ndl_asm_parse_line(const char *src, ndl_asm_result *res) {

    long int off;
    if (IS_TOKEN_WS(src[0])) {

        off = ndl_asm_parse_eat_ws(src, res);

        return ndl_asm_parse_line(src + off, res) + off;

    } else if (IS_TOKEN_COMMENT(src[0])) {

        return ndl_asm_parse_eat_comment(src, res);

    } else if (IS_TOKEN_EOL(src[0])) {

        return 1;

    } else if (IS_TOKEN_EOS(src[0])) {

        return 0;

    } else if (IS_TOKEN_SYMBOL(src[0])) {

        return ndl_asm_parse_nline(src, res);

    } else {

        PARSEFAIL("Expected whitespace, comment, end of line, end of string, symbol, or label.");
    }
}

ndl_asm_result ndl_asm_parse(const char *source, ndl_graph *using) {

    ndl_asm_result res;

    res.src = source;
    res.graph = using;
    res.line = res.column = 0;

    /* Allocate a graph if necessary. */
    if (using == NULL) {
        res.graph = ndl_graph_init();
        if (res.graph == NULL) {
            res.msg = "Failed to allocate graph: internal error.";
            return res;
        }
    }

    /* Initialize the graph datastructure. */
    if (ndl_asm_parse_init(&res) != 0) {

        ndl_asm_parse_kill(&res);
        if (using == NULL)
            ndl_graph_kill(res.graph);

        res.graph = NULL;
        res.msg = "Failed to create root nodes: internal error.";
        return res;
    }

    /* Parse each line. */
    while (source[0] != '\0') {

        long int used = ndl_asm_parse_line(source, &res);
        if (used <= 0) {
            ndl_asm_parse_kill(&res);
            if (using == NULL)
                ndl_graph_kill(res.graph);
            if (res.msg == NULL)
                res.msg = "Unknown error.";
            res.graph = NULL;
            return res;
        }
        res.line++;
        res.column = 0;
        source += used;
    }

    /* Resolve any forward/bad references, then free badref list. */
    if (res.badref_head == NDL_NULL_REF)
        return res;

    while (res.badref_head != NDL_NULL_REF) {

        ndl_value inst   = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("inst    "));
        ndl_value label  = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("label   "));
        ndl_value symbol = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("symbol  "));
        ndl_value line   = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("line    "));
        ndl_value column = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("column  "));
        if (((inst.type != EVAL_REF) || (symbol.type != EVAL_SYM) || (label.type != EVAL_SYM) ||
                          (inst.ref == NDL_NULL_REF) || (symbol.sym == NDL_NULL_SYM) || (label.sym == NDL_NULL_SYM) ||
                          (line.type != EVAL_INT) || (line.type != EVAL_INT))) {
            ndl_asm_parse_kill(&res);
            if (using == NULL)
                ndl_graph_kill(res.graph);
            res.msg = "Failed to load delayed label resolution node: internal error. No line number available.";
            res.line = res.column = 0;
            res.graph = NULL;
            return res;
        }

        ndl_value dest = ndl_graph_get(res.graph, res.label_table, label.sym);
        if (dest.type != EVAL_REF) {
            ndl_asm_parse_kill(&res);
            if (using == NULL)
                ndl_graph_kill(res.graph);
            res.msg = "Failed to find label in delayed reference. Possibly internal error, probably bad label.";
            res.graph = NULL;
            res.line = line.num;
            res.column = column.num;
            return res;
        }

        int err = ndl_graph_set(res.graph, inst.ref, symbol.sym, NDL_VALUE(EVAL_REF, ref=dest.ref));
        ndl_value next = ndl_graph_get(res.graph, res.badref_head, NDL_SYM("brefnext"));
        if ((next.type != EVAL_REF) || (err != 0)) {
            ndl_asm_parse_kill(&res);
            if (using == NULL)
                ndl_graph_kill(res.graph);
            res.msg = "Failed to resolve delayed reference: internal error.";
            res.graph = NULL;
            res.line = line.num;
            res.column = column.num;
            return res;
        }

        res.badref_head = next.ref;
    }

    int err = ndl_graph_del(res.graph, res.root, NDL_SYM("brefhead"));
    if (err != 0) {
        ndl_asm_parse_kill(&res);
        if (using == NULL)
            ndl_graph_kill(res.graph);
        res.msg = "Failed to delete resolved delayed references: internal error.";
        res.graph = NULL;
        res.line = res.column = 0;
        return res;
    }

    ndl_graph_clean(res.graph);

    return res;
}

void ndl_asm_print_err(ndl_asm_result res) {

    if (res.msg == NULL) {

        fprintf(stderr, "Successfully assembled program.\n");
        fprintf(stderr, "Root node, graph pointer: %ld@%p\n", res.root, (void *) res.graph);
    } else {

        fprintf(stderr, "Failed to assemble program.\n");
        fprintf(stderr, "%03ld:%03ld: %s\n", res.line, res.column, res.msg);

        if (res.src == NULL) {
            fprintf(stderr, "Failed to print line: Source not included.\n");
        }

        long int line = res.line;
        const char *curr = res.src;
        while (line > 0) {

            while ((*curr != '\n') && (*curr != '\r') && (*curr != '\0'))
                curr++;

            if (*curr == '\0') {
                fprintf(stderr, "Failed to print line: line number is out of range.\n");
                return;
            }

            curr++;
            line--;
        }

        const char *base = curr;

        while ((*curr != '\n') && (*curr != '\r') && (*curr != '\0'))
            curr++;

        char *nline = malloc((unsigned long) (curr - base + 2));
        memcpy(nline, base, (unsigned long) (curr - base));
        nline[curr - base] = '\n';
        nline[curr - base + 1] = '\0';

        fprintf(stderr, "----|Start Error|------------------\n");
        fputs(nline, stderr);
        if ((res.column < (curr - base)) && (res.column > 0)) {
            memset(nline, ' ', (unsigned long) (curr - base));
            nline[res.column] = '^';
            fputs(nline, stderr);
        }
        fprintf(stderr, "------|End Error|------------------\n");
    }

    fprintf(stderr, "--|Start Program|------------------\n");
    fprintf(stderr, "%s", res.src);
    fprintf(stderr, "----|End Program|------------------\n");

    return;
}
