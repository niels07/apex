#ifndef APX_LEX_H
#define APX_LEX_H

#include "io.h"
#include "value.h"

typedef struct ApexLexer ApexLexer;

typedef enum ApexToken {
  	APEX_TOKEN_INT = 257,
    APEX_TOKEN_FLT,
    APEX_TOKEN_STR,
    APEX_TOKEN_ID,
    APEX_TOKEN_INC,
    APEX_TOKEN_DEC,
    APEX_TOKEN_EQ,
    APEX_TOKEN_NE,
    APEX_TOKEN_LE,
    APEX_TOKEN_GE,
    APEX_TOKEN_AND,
    APEX_TOKEN_OR,
    APEX_TOKEN_LSHIFT,
    APEX_TOKEN_RSHIFT,
    APEX_TOKEN_ASGN_ADD,
    APEX_TOKEN_ASGN_SUB,
    APEX_TOKEN_ASGN_MUL,
    APEX_TOKEN_ASGN_DIV,
    APEX_TOKEN_ASGN_MOD,
    APEX_TOKEN_ASGN_LSHIFT,
    APEX_TOKEN_ASGN_RSHIFT,
    APEX_TOKEN_ASGN_AND,
    APEX_TOKEN_ASGN_XOR,
    APEX_TOKEN_ASGN_OR,
    APEX_TOKEN_IF,
    APEX_TOKEN_ELSE,
    APEX_TOKEN_ELIF,
    APEX_TOKEN_WHILE,
    APEX_TOKEN_DECL_INT,
    APEX_TOKEN_DECL_FLT,
    APEX_TOKEN_DECL_STR,
    APEX_TOKEN_MODULE,
    APEX_TOKEN_IMPORT,
    APEX_TOKEN_END,
    APEX_TOKEN_END_STMT,
    APEX_TOKEN_EOS
} ApexToken;

extern ApexLexer *apex_lexer_new(ApexStream *);
extern const char *apex_lexer_get_token_str(ApexToken);
extern ApexToken apex_lexer_next_token(ApexLexer *);
extern char *apex_lexer_get_str(ApexLexer *);
extern ApexToken apex_lexer_get_token(ApexLexer *);
extern ApexToken apex_lexer_peek(ApexLexer *);
extern int apex_lexer_get_lineno(ApexLexer *);
extern ApexValue *apex_lexer_get_value(ApexLexer *);

#endif
