#ifndef APEX_LEX_H
#define APEX_LEX_H

#include <stdbool.h>
#include "apexStr.h"

/**
 * Represents a source location in the source code.
 */
typedef struct {
    int lineno;            /**< Line number in the source file */
    const char *filename;  /**< Name of the source file */
} SrcLoc;

/**
 * Enumeration of token types recognized by the lexer.
 */
typedef enum {
    TOKEN_IDENT,           /** Identifier token */
    TOKEN_INT,             /** Integer token */
    TOKEN_DBL,             /** Double (floating point) token */
    TOKEN_STR,             /** String token */
    TOKEN_NULL,            /** Null token */
    TOKEN_IF,              /** 'if' keyword */
    TOKEN_ELIF,            /** 'elif' keyword */
    TOKEN_ELSE,            /** 'else' keyword */
    TOKEN_FN,              /** 'fn' keyword */
    TOKEN_FOR,             /** 'for' keyword */
    TOKEN_WHILE,           /** 'while' keyword */
    TOKEN_FOREACH,         /** 'foreach' keyword */
    TOKEN_IN,              /** 'in' keyword */
    TOKEN_RETURN,          /** 'return' keyword */
    TOKEN_PLUS,            /** Plus operator '+' */
    TOKEN_MINUS,           /** Minus operator '-' */
    TOKEN_STAR,            /** Multiplication operator '*' */
    TOKEN_SLASH,           /** Division operator '/' */
    TOKEN_PERCENT,         /** Modulo operator '%' */
    TOKEN_PLUS_PLUS,       /** Increment operator '++' */
    TOKEN_MINUS_MINUS,     /** Decrement operator '--' */
    TOKEN_EQUAL,           /** Assignment operator '=' */
    TOKEN_PLUS_EQUAL,      /** Addition assignment operator '+=' */
    TOKEN_MINUS_EQUAL,     /** Subtraction assignment operator '-=' */
    TOKEN_STAR_EQUAL,      /** Multiplication assignment operator '*=' */
    TOKEN_SLASH_EQUAL,     /** Division assignment operator '/=' */
    TOKEN_MOD_EQUAL,       /** Modulo assignment operator '%=' */
    TOKEN_EQUAL_EQUAL,     /** Equality operator '==' */
    TOKEN_NOT_EQUAL,       /** Inequality operator '!=' */
    TOKEN_LESS,            /** Less than operator '<' */
    TOKEN_GREATER,         /** Greater than operator '>' */
    TOKEN_LESS_EQUAL,      /** Less than or equal operator '<=' */
    TOKEN_GREATER_EQUAL,   /** Greater than or equal operator '>=' */
    TOKEN_NOT,             /** Logical NOT operator '!' */
    TOKEN_AMP,             /** Bitwise AND operator '&' */
    TOKEN_AND,             /** Logical AND operator '&&' */
    TOKEN_PIPE,            /** Bitwise OR operator '|' */
    TOKEN_OR,              /** Logical OR operator '||' */
    TOKEN_QUESTION,        /** Ternary question mark '?' */
    TOKEN_BREAK,           /** 'break' keyword */
    TOKEN_CONTINUE,        /** 'continue' keyword */
    TOKEN_LPAREN,          /** Left parenthesis '(' */
    TOKEN_RPAREN,          /** Right parenthesis ')' */
    TOKEN_LBRACE,          /** Left brace '{' */
    TOKEN_RBRACE,          /** Right brace '}' */
    TOKEN_LBRACKET,        /** Left bracket '[' */
    TOKEN_RBRACKET,        /** Right bracket ']' */
    TOKEN_COMMA,           /** Comma ',' */
    TOKEN_SEMICOLON,       /** Semicolon ';' */
    TOKEN_TRUE,            /** 'true' keyword */
    TOKEN_FALSE,           /** 'false' keyword */
    TOKEN_ARROW,           /** Arrow '=>' */
    TOKEN_INCLUDE,         /** 'include' keyword */
    TOKEN_DOT,             /** Dot '.' */
    TOKEN_COLON,           /** Colon ':' */
    TOKEN_SWITCH,          /** 'switch' keyword */
    TOKEN_CASE,            /** 'case' keyword */
    TOKEN_DEFAULT,         /** 'default' keyword */
    TOKEN_EOF              /** End of file token */
} TokenType;

/**
 * Represents a token produced by the lexer.
 */
typedef struct {
    TokenType type;       /** The type of the token */
    ApexString *str;      /** The string representation of the token */
    SrcLoc srcloc;        /** The source location of the token */
} Token;

/**
 * Represents the lexer state while processing source code.
 */
typedef struct {
    char *source;        /** The source code being lexed */
    int length;          /** Length of the source code */
    int position;        /** Current position in the source code */
    SrcLoc srcloc;       /** Current source location */
} Lexer;

extern void apexLex_feedline(Lexer *lexer, const char *line);
extern ApexString *get_token_str(TokenType type);
extern void init_lexer(Lexer *lexer, const char *filename, char *source);
extern Token *get_next_token(Lexer *lexer);
extern void free_token(Token *token);

#endif