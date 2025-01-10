#ifndef TOKEN_H
#define TOKEN_H

#include "apexStr.h"
#include "srcloc.h"

typedef enum {
    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_DBL,
    TOKEN_STR,
    TOKEN_NULL,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_FN,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    TOKEN_EQUAL,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_NOT,
    TOKEN_AMP,
    TOKEN_AND,
    TOKEN_PIPE,
    TOKEN_OR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_ARROW,
    TOKEN_INCLUDE,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    ApexString *str;
    ParseState parsestate;
} Token;

#endif
