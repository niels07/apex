#ifndef APX_LEX_H
#define APX_LEX_H

#include <stdio.h>
#include "value.h"
#include "lexer.h"

typedef enum ApexLexer_token {
    APEX_LEXER_TOKEN_INT = 257,
    APEX_LEXER_TOKEN_FLT,
    APEX_LEXER_TOKEN_STR,
    APEX_LEXER_TOKEN_BOOL,
    APEX_LEXER_TOKEN_TRUE,
    APEX_LEXER_TOKEN_FALSE,
    APEX_LEXER_TOKEN_NOT,
    APEX_LEXER_TOKEN_ID,
    APEX_LEXER_TOKEN_INC,
    APEX_LEXER_TOKEN_DEC,
    APEX_LEXER_TOKEN_ASGN_ADD,
    APEX_LEXER_TOKEN_ASGN_SUB,
    APEX_LEXER_TOKEN_ASGN_MUL,
    APEX_LEXER_TOKEN_ASGN_DIV,
    APEX_LEXER_TOKEN_EQUALS,
    APEX_LEXER_TOKEN_NOT_EQUAL,
    APEX_LEXER_TOKEN_LESS,
    APEX_LEXER_TOKEN_LESS_EQUAL,
    APEX_LEXER_TOKEN_GREATER,
    APEX_LEXER_TOKEN_GREATER_EQUAL,
    APEX_LEXER_TOKEN_ADD,
    APEX_LEXER_TOKEN_DEF,
    APEX_LEXER_TOKEN_SUB,
    APEX_LEXER_TOKEN_MUL,
    APEX_LEXER_TOKEN_DIV,
    APEX_LEXER_TOKEN_AND,
    APEX_LEXER_TOKEN_OR,
    APEX_LEXER_TOKEN_IF,
    APEX_LEXER_TOKEN_ELSE,
    APEX_LEXER_TOKEN_ELIF,
    APEX_LEXER_TOKEN_WHILE,
    APEX_LEXER_TOKEN_DECL_INT,
    APEX_LEXER_TOKEN_DECL_FLT,
    APEX_LEXER_TOKEN_DECL_BOOL,
    APEX_LEXER_TOKEN_DECL_STR,
    APEX_LEXER_TOKEN_DECL_FUNC,
    APEX_LEXER_TOKEN_MODULE,
    APEX_LEXER_TOKEN_IMPORT,
    APEX_LEXER_TOKEN_EOF,
    APEX_LEXER_TOKEN_END_STMT,
    APEX_LEXER_TOKEN_NULL_TERM,
    APEX_LEXER_TOKEN_VOID,
    APEX_LEXER_TOKEN_RETURN,
    APEX_LEXER_TOKEN_END
} ApexLexer_token;

typedef struct ApexLexer_token_info {
    ApexValue value;
    ApexLexer_token token;
    int lineno;
} ApexLexer_token_info;

typedef struct ApexLexer_trie_node {
    struct ApexLexer_trie_node *children[26];
    int keyword_index;
} ApexLexer_trie_node;

typedef struct ApexLexer {
    ApexLexer_token_info cur;
    ApexLexer_token_info next;
    int lineno;
    FILE *file;
    ApexLexer_trie_node *trie_root;
    char c;
    struct {
        size_t len;
        size_t size;
        char *str;
    } buf;
} ApexLexer;

#define LEXER_TOKEN_INFO(lexer) lexer->next
#define LEXER_TOKEN_INFO_INT(lexer) LEXER_TOKEN_INFO(lexer).value.data.int_val
#define LEXER_TOKEN_INFO_FLT(lexer) LEXER_TOKEN_INFO(lexer).value.data.flt_val
#define LEXER_TOKEN_INFO_STR(lexer) LEXER_TOKEN_INFO(lexer).value.data.str_val

extern ApexLexer *ApexLexer_new(FILE *);
extern void ApexLexer_free(ApexLexer *);
extern const char *ApexLexer_get_token_str(ApexLexer_token);
extern ApexLexer_token ApexLexer_next_token(ApexLexer *);
extern ApexLexer_token ApexLexer_get_token(ApexLexer *);
extern ApexLexer_token ApexLexer_next_token(ApexLexer *);
extern ApexLexer_token ApexLexer_peek(ApexLexer *);
extern int ApexLexer_get_lineno(ApexLexer *);
extern char *ApexLexer_get_str(ApexLexer *);
extern int ApexLexer_get_int(ApexLexer *);
extern float ApexLexer_get_flt(ApexLexer *);
extern bool ApexLexer_get_bool(ApexLexer *);
extern ApexLexer_token ApexLexer_get_token(ApexLexer *);
extern ApexLexer_token ApexLexer_peek(ApexLexer *);
extern int ApexLexer_get_lineno(ApexLexer *);
extern ApexValue *ApexLexer_get_value(ApexLexer *);
extern const char *ApexLexer_get_token_str(ApexLexer_token);

#endif
