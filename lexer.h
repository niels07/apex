#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char *source;
    int length;
    int position;
    int line;
} Lexer;

extern char *get_token_str(TokenType type);
extern void init_lexer(Lexer *lexer, const char *source);
extern Token *get_next_token(Lexer *lexer);
extern void free_token(Token *token);

#endif