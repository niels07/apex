#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "srcloc.h"

typedef struct {
    const char *source;
    int length;
    int position;
    SrcLoc srcloc;
} Lexer;

extern char *get_token_str(TokenType type);
extern void init_lexer(Lexer *lexer, const char *filename, const char *source);
extern Token *get_next_token(Lexer *lexer);
extern void free_token(Token *token);

#endif