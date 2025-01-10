#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "ast.h"
#include "token.h"
#include "lexer.h"

typedef struct {
    Lexer *lexer;
    Token *current_token;
    bool allow_incomplete;
} Parser;

extern void init_parser(Parser *parser, Lexer *lexer);
extern AST *parse_program(Parser *parser);
extern void free_parser(Parser *parser);

#endif