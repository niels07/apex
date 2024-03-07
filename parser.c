#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "malloc.h"
#include "error.h"

#define TOKEN_SIZE 255
#define TOKEN_INCR 64

static void check_token_size(ApexLexer_token **tokens, size_t token_count) {
    if (token_count >= TOKEN_SIZE) {
        apex_realloc(*tokens, (TOKEN_SIZE + TOKEN_INCR) * sizeof(ApexLexer_token));
    }
}

static ApexLexer_token *get_tokens(ApexLexer *lexer, size_t token_count) {
    ApexLexer_token *tokens = apex_malloc(TOKEN_SIZE * sizeof(ApexLexer_token)); 
    int i = 0;
    for (;;) {
        ApexLexer_token token = ApexLexer_get_token(lexer);
        if (token == APEX_LEXER_TOKEN_NULL_TERM) {
            break;
        }
        check_token_size(&tokens, token_count);
        tokens[i++] = token;
    } 
    return tokens;
}

void ApexParser_parse(ApexLexer *lexer) {
    size_t token_count = 0;
    ApexLexer_token *tokens = get_tokens(lexer, token_count);
    free(tokens);
}

