#ifndef APEX_PARSER_H
#define APEX_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct ApexParser ApexParser;

extern ApexParser *apex_parser_new(ApexLexer *, ApexAst *);
void apex_parser_parse(ApexParser *);

#endif
