#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    APEX_TOKEN_EOF,
    APEX_TOKEN_IDENTIFIER,
    APEX_TOKEN_NUMBER,
    APEX_TOKEN_PRINT,
    APEX_TOKEN_EQUALS,
    APEX_TOKEN_PLUS,
    APEX_TOKEN_MINUS,
    APEX_TOKEN_MULTIPLY,
    APEX_TOKEN_DIVIDE,
    APEX_TOKEN_LPAREN, // Left parenthesis '('
    APEX_TOKEN_RPAREN, // Right parenthesis ')'
    APEX_TOKEN_COMMA,  // Comma ','
    APEX_TOKEN_SEMICOLON // Semicolon ';'
} ApexTokenType;

typedef struct {
    ApexTokenType type;
    char* Data;
} ApexAstToken;
