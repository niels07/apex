#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "io.h"
#include "lexer.h"

static void lex(ApexLexer *lexer) {
    ApexToken tk;
    const char *tkstr;

    do {
        tk = apex_lexer_next_token(lexer);
        tkstr = apex_lexer_get_token_str(tk);

        switch (tk) {
        case APEX_TOKEN_STR:
        case APEX_TOKEN_ID:
            printf("[%d] %s (%s)\n", tk, tkstr, apex_lexer_get_str(lexer));
            break;

        default:
            printf("[%d] %s\n", tk, tkstr);
            break;
        }
    } while (tk != APEX_TOKEN_EOS);
}

int main(void) {
    ApexStream *stream = apex_stream_open("test.apex");
    ApexLexer *lexer = apex_lexer_new(stream);
    lex(lexer);

    return 0;
}

