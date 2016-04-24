#ifndef NODEL_ASM_H
#define NODEL_ASM_H

#include "graph.h"

/* Interpret an ./INST like assembly language and
 * generate the described function's graph datastructure.
 * 
 * INSTRUCTION SET
 *
 * Labels are references to the next instruction.
 * Empty lines are ignored.
 * Comments are parsed out.
 * Whitespace is ignored.
 * Instructions are a list of symbols.
 * Instructions may use ".", "->", or "," to separate args.
 * Symbols may not be more than eight chars long.
 * You may 'attach' extra key/value pairs to an instruction by
 * adding a '|' to the end of the line, followed by a series
 * of space separated key=value pairs. These may overwrite
 * default opcode, syma, symb, next options.
 *
 * EXAMPLE
 *
 * load instpntr.sum -> sum | sum=0.0
 * loop:
 * fadd sum, 3.1415 -> sum
 * sub counter, 1 -> counter # comment
 * branch counter, 0 | gt=:loop
 * print sum
 *
 * Note: you may or may not have space between args, '=', '|', ',', '->',
 * but you *MUST* have a space after integers and before '.'s.
 * 3.self will be an error, and 3.14 will be interpreted as a float.
 *
 * INSTRUCTION SET CFG
 *
 * ASM: LINE+
 * LINE : WS* (LABEL | OPCODE)? EMPTY \n
 * EMPTY : WS* COMMENT?
 * WS : " " | "\t"
 * COMMENT : "#" [^\n]*
 * LABEL : SYM ":"
 * OPCODE : MAINOP WS+ EXTRAOP?
 * MAINOP : SYM (WS+ OBJ (WS* SEPARATOR WS* OBJ)*)?
 * SYM : [a-zA-Z_][a-zA-Z_0-9]*
 * OBJ : SYM | FLOAT | INT | REF
 * FLOAT : "-"? [0-9]+ "." [0-9]+
 * INT : "-"? [0-9]+
 * REF : SYM ":"
 * SEPARATOR : "," | "." | "->"
 * EXTRAOP: "|" (WS+ SYM WS* "=" WS* OBJ)*
 */

/* Results of an attempted assembly.
 * Stores the source text, the graph,
 * references to some of the instantiated nodes,
 * and the error message with line/column data.
 *
 * On error, msg is not NULL.
 */
typedef struct ndl_asm_result_s {

    const char *src;

    ndl_graph *graph;

    ndl_ref root;
    ndl_ref inst_head, inst_tail;
    ndl_ref label_table, badref_head;

    const char *msg;
    long int line, column;

} ndl_asm_result;

ndl_asm_result ndl_asm_parse(const char *source, ndl_graph *using);
void ndl_asm_print_err(ndl_asm_result result);

#endif /* NODEL_ASM_H */
