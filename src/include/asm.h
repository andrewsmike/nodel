#ifndef NODEL_ASM_H
#define NODEL_ASM_H

#include "graph.h"

/* Interpret an ./INST like assembly language and
 * generate the described nodel function datastructure.
 * 
 * Allows you to parse a program string and generate an
 * intermediate datastructure. You can then 'write' this
 * datastructure into any graph, and get the resulting root node.
 *
 *
 * INSTRUCTION SET
 * Labels are references to the next instruction.
 * Empty lines are ignored.
 * Comments are parsed out.
 * Instructions are a list of symbols.
 * Instructions may use ".", "->", or "," to separate args.
 * Symbols may not be more than eight chars long.
 * You may 'attach' extra key/value pairs to an instruction by
 * adding a '|' to the end of the line, followed by a series
 * of comma separated key=type(value) pairs.
 *
 * EXAMPLE
 *
 * load self.instpntr.sum -> self.sum | sum=FLOAT(0)
 * loop:
 * fadd sum, FLOAT(3.1415) -> sum
 * sub counter, INT(-1) -> counter
 * branch counter, INT(0) -> break, break, loop
 * break:
 * print sum
 *
 * INSTRUCTION SET CFG
 *
 * ASM: LINE+
 * LINE : (EMPTY | LABEL | OPCODE) \n
 * EMPTY : WS* COMMENT?
 * WS : " " | "\t"
 * COMMENT : "#" [^\n]*
 * LABEL : SYM ":" EMPTY
 * OPCODE : MAINOP WS+ EXTRAOP? EMPTY
 * MAINOP : SYM (WS+ OBJ (WS+ SEPARATOR WS+ OBJ)*)?
 * SYM : [a-zA-Z_][a-zA-Z_0-9]*
 * OBJ : SYM | (TYPE "(" (FPCONST | INTCONST | REFCONST) ")")
 * TYPE : "FLOAT" | "INT" | "REF"
 * FPCONST : "-"? [0-9]+ "." [0-9]+
 * INTCONST : "-"? [0-9]+
 * REFCONST : SYM
 * SEPARATOR : "," | "." | "->"
 * EXTRAOP: "|" WS+ SYM WS* "=" WS* OBJ ("," WS* SYM WS* "=" WS* OBJ)*
 */

typedef struct ndl_asm_script_s ndl_asm_script;

ndl_asm_script *ndl_asm_parse(const char *source);

ndl_ref ndl_asm_gen(ndl_graph *graph, ndl_asm_script *script);

#endif /* NODEL_ASM_H */
