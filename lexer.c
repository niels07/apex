#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "error.h"
#include "malloc.h"
#include "lexer.h"
#include "io.h"

#define BUF_INIT_SIZE 256
#define BUF_GROW_SIZE 128

#define NEXT_CH(lexer) ((lexer)->c = apex_stream_getch((lexer)->stream))

typedef struct tokenInfo {
    ApexValue value;
    ApexToken token; 
    int lineno;
} TokenInfo;

struct ApexLexer {
    TokenInfo cur;
    TokenInfo next;
    int lineno;
    ApexStream *stream;
    char c;
    struct {
        size_t len;
        size_t size;
        char *str;
    } buf;
};

static const char *KEYWORDS[] = {
    "module", "import",
    "if","else", "elif", "while",
    "int", "flt", "str", "end", NULL
};

static const int KEYWORD_TOKENS[] = {
    APEX_TOKEN_MODULE,
    APEX_TOKEN_IMPORT,
    APEX_TOKEN_IF, 
    APEX_TOKEN_ELSE, 
    APEX_TOKEN_ELIF, 
    APEX_TOKEN_WHILE,
    APEX_TOKEN_DECL_INT,
    APEX_TOKEN_DECL_FLT,
    APEX_TOKEN_DECL_STR,
    APEX_TOKEN_END,
};

static const char *TOKEN_STR[] = {
    "integer", "float", "string", "identifier",
    "++", "--", 
    "==", "!=", "<=", ">=",
    "&&", "||",
    "<<", ">>",
    "+=", "-=", "*=", "/=", "%=", 
    ">>=", "<<=", "&=", "^=", "|=",
    "if", "else", "elif", "while",
    "int", "flt", "str",
    "module", "import",
    "end",
    "end of statement",
    "end of stream"
};

static void reset_buf(ApexLexer *lexer) {
    lexer->buf.len = 0;
    lexer->buf.size = BUF_INIT_SIZE;
    lexer->buf.str = apex_malloc(BUF_INIT_SIZE);
}

static void check_buf_size(ApexLexer *lexer) {
    if (lexer->buf.len == lexer->buf.size) {
        lexer->buf.size += BUF_GROW_SIZE;
        lexer->buf.str = apex_realloc(lexer->buf.str, lexer->buf.size);
    }
}

static void set_tk(ApexLexer *lexer, TokenInfo *ti, ApexToken tk) {
    ti->token = tk; 
    ti->lineno = lexer->lineno;
}

static char *finish_buf(ApexLexer *lexer) {
    char *buf = lexer->buf.str;

    buf[lexer->buf.len] = '\0';
    reset_buf(lexer);
    return buf;
}

static void save_ch(ApexLexer *lexer) {
    lexer->buf.str[lexer->buf.len++] = lexer->c;
    check_buf_size(lexer);
}

static void save_and_next_ch(ApexLexer *lexer) {
    lexer->buf.str[lexer->buf.len++] = lexer->c;
    check_buf_size(lexer);
    NEXT_CH(lexer);
} 

static void get_eq(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_EQ);
        break;

    default:
        set_tk(lexer, ti, '=');
        break;
    }   
}

static void get_add(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '+':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_INC);
        break;

    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_ASGN_ADD);
        break;

    default:
        set_tk(lexer, ti, '+');
        break;
    }
}

static void get_sub(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '-':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_DEC);
        break;

    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_ASGN_SUB);
        break;

    default:
        set_tk(lexer, ti, '-');
        break;
    }
}

static void get_mul(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_ASGN_MUL);
        break;

    default:
        set_tk(lexer, ti, '*');
        break;
    }
}

static void get_div(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_ASGN_DIV);
        break;

    default:
        set_tk(lexer, ti, '/');
        break;
    }
}

static void get_not(ApexLexer *lexer, TokenInfo *ti) {
    NEXT_CH(lexer);
    switch (lexer->c) {
    case '=':
        NEXT_CH(lexer);
        set_tk(lexer, ti, APEX_TOKEN_NE);
        break;

    default:
        set_tk(lexer, ti, '!');
        break;
    }
}

static void get_number(ApexLexer *lexer, TokenInfo *ti) {
    char *number;

    set_tk(lexer, ti, APEX_TOKEN_INT);
    for (;;) {
        if (lexer->c == '.' 
        && ti->token == APEX_TOKEN_FLT) {
            set_tk(lexer, ti, APEX_TOKEN_FLT);
            save_and_next_ch(lexer); 
        } else if (isdigit(lexer->c)) {
            save_and_next_ch(lexer); 
        } else {
            break;
        }
    }
    number = finish_buf(lexer);

    if (ti->token == APEX_TOKEN_INT) {
        APEX_VALUE_INT(&ti->value) = atoi(number);
    } else {
        APEX_VALUE_FLT(&ti->value) = atof(number);
    }
}

static void get_str(ApexLexer *lexer, TokenInfo *ti) {
    char prev = 0;

    for (;;) {
        NEXT_CH(lexer);
        if (lexer->c == '"') {
            if (prev != '\\') {
                NEXT_CH(lexer);
                break;
            }
        }
        prev = lexer->c;
        save_ch(lexer);
    }
    APEX_VALUE_STR(&ti->value) = finish_buf(lexer);
    set_tk(lexer, ti, APEX_TOKEN_STR);
}

static void find_name_token(ApexLexer *lexer, TokenInfo *ti) {
    size_t i;
    
    for (i = 0; KEYWORDS[i]; i++) { 
        if (strncmp(KEYWORDS[i], lexer->buf.str, lexer->buf.len) == 0) {
            set_tk(lexer, ti, KEYWORD_TOKENS[i]);
            return;
        }
    }
    APEX_VALUE_STR(&ti->value) = finish_buf(lexer);
    set_tk(lexer, ti, APEX_TOKEN_ID);
}

static void get_name(ApexLexer *lexer, TokenInfo *ti) {
    while (lexer->c == '_' || isalnum(lexer->c)) {
        save_and_next_ch(lexer);
    }
    find_name_token(lexer, ti);
}

static void next_token(ApexLexer *lexer, TokenInfo *ti) {
    char c = lexer->c;
    switch (c) {
    case '=':
        get_eq(lexer, ti);
        return; 

    case '+':
        get_add(lexer, ti);
        return; 

    case '-':
        get_sub(lexer, ti);
        return;

    case '*':
        get_mul(lexer, ti);
        return;

    case '/':
        get_div(lexer, ti);
        return;

    case '"':
        get_str(lexer, ti);
        return;

    case '\n': 
        do {
            NEXT_CH(lexer);
            lexer->lineno++;
        } while (lexer->c == '\n');
        set_tk(lexer, ti, APEX_TOKEN_END_STMT);
        return;

    case '(':
    case ')':
    case ':':
        NEXT_CH(lexer);
        set_tk(lexer, ti, c);
        return;

    case '\0':
        set_tk(lexer, ti, APEX_TOKEN_EOS);
        return;

    case '!':
        get_not(lexer, ti);
        return;

    case '\t':
    case ' ':
        NEXT_CH(lexer);
        next_token(lexer, ti);
        return;

    default:
        break;
    }

    if (isdigit(lexer->c)) {
        get_number(lexer, ti);
        return;
    }

    if (lexer->c == '_' || isalpha(lexer->c)) {
        get_name(lexer, ti);
        return;
    }

    apex_error_throw(
        APEX_ERROR_CODE_SYNTAX,
        "unexpected character '%c' on line %d",
        lexer->c,
        lexer->lineno);
}

ApexLexer *apex_lexer_new(ApexStream *stream) {
    ApexLexer *lexer = APEX_NEW(ApexLexer); 

    lexer->lineno = 1;
    lexer->stream = stream;
    reset_buf(lexer);
    NEXT_CH(lexer);
    next_token(lexer, &lexer->next);
    return lexer;
}

ApexToken apex_lexer_next_token(ApexLexer *lexer) {
    if (lexer->next.token == APEX_TOKEN_EOS) {
        return APEX_TOKEN_EOS;
    }
    lexer->cur = lexer->next;
    next_token(lexer, &lexer->next);
    return lexer->cur.token;
}

const char *apex_lexer_get_token_str(ApexToken tk) {
    static char single_char_token[1];
    char *p;

    for (p = "()+-*/%?:!"; *p; p++) {
        if (tk != *p) {
            continue;
        }
        single_char_token[0] = *p;
        single_char_token[1] = '\0';
        return single_char_token; 
    }
    return TOKEN_STR[tk - 257];
}

char *apex_lexer_get_str(ApexLexer *lexer) {
    return APEX_VALUE_STR(&lexer->cur.value);
}

ApexToken apex_lexer_get_token(ApexLexer *lexer) {
    return lexer->cur.token;
}

ApexToken apex_lexer_peek(ApexLexer *lexer) {
    return lexer->next.token;
}

int apex_lexer_get_lineno(ApexLexer *lexer) {
    return lexer->cur.lineno;
}

ApexValue *apex_lexer_get_value(ApexLexer *lexer) {
    return &lexer->cur.value;
}


