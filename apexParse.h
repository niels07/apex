#ifndef APEX_PARSE_H
#define APEX_PARSE_H

#include <stdbool.h>
#include "apexAST.h"
#include "apexLex.h"

/**
 * Represents a parser structure which holds the state of parsing process.
 */
typedef struct {
    Lexer *lexer;            /** Pointer to the Lexer used for token generation. */
    Token *current_token;    /** Pointer to the current token being processed. */
    bool allow_incomplete;   /** Flag indicating if incomplete code is allowed. */
} Parser;

extern void init_parser(Parser *parser, Lexer *lexer);
extern AST *parse_program(Parser *parser);
extern void free_parser(Parser *parser);

#endif