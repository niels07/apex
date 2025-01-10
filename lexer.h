#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include "apexStr.h"
#include "token.h"
#include "srcloc.h"

typedef struct {
    char *source;
    int length;
    int position;
    ParseState parsestate;
} Lexer;

extern void apexLex_feedline(Lexer *lexer, const char *line);
extern ApexString *get_token_str(TokenType type);
extern void init_lexer(Lexer *lexer, const char *filename, char *source);
extern Token *get_next_token(Lexer *lexer);
extern void free_token(Token *token);

#endif