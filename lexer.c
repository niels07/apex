#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "error.h"
#include "malloc.h"
#include "lexer.h"
#include "value.h"

#define BUF_INIT_SIZE 256
#define BUF_GROW_SIZE 128

enum {
    KEYWORD_IF = 0,
    KEYWORD_ELSE,
    KEYWORD_FUNCTION,
    KEYWORD_IMPORT,
    KEYWORD_INT,
    KEYWORD_FLT,
    KEYWORD_BOOL,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_STR,
    KEYWORD_VOID,
    KEYWORD_RETURN,
    KEYWORD_END
};

const char *KeyWords[] = {
    "if",
    "else",
    "function",
    "import",
    "int",
    "float",
    "bool",
    "true",
    "false",
    "str",
    "void",
    "return",
    "end"
};

#define ALPHABET_SIZE 26
#define APEX_BUF_SIZE 256

#define STR_VAL(lexer) lexer->cur.value.data.str_val
#define INT_VAL(lexer) lexer->cur.value.data.int_val
#define FLT_VAL(lexer) lexer->cur.value.data.flt_val
#define FUNC_VAL(lexer) lexer->cur.value.data.func_val
#define BOOL_VAL(lexer) lexer->cur.value.data.bool_val

static void next_token(ApexLexer *lexer);

static ApexLexer_trie_node *create_trie_node() {
    int i;
    ApexLexer_trie_node *node = APEX_NEW(ApexLexer_trie_node);
    for (i = 0; i < ALPHABET_SIZE; i++) {
        node->children[i] = NULL;
    }
    return node;
}

static void insert_word(ApexLexer_trie_node *root, const char *word, int keyword_index) {
    ApexLexer_trie_node *node = root;
    while (*word) {
        int index = *word - 'a'; /* Get the index for the current character */
        if (!node->children[index]) { /* If the node has not yet been initialized */
            node->children[index] = create_trie_node(); /* Initialize the node with a new trie node */
        }
        node->keyword_index = keyword_index;
        node = node->children[index];
        word++;
    }
}

static void destroy_trie(ApexLexer_trie_node *root) {
    int i;
    if (root) {
        for (i = 0; i < ALPHABET_SIZE; i++) {
            destroy_trie(root->children[i]);
        }
        apex_free(root);
    }
}

static int search_keyword(ApexLexer_trie_node *root, const char *word) {
    ApexLexer_trie_node *node = root;
    while (*word) {
        int index = *word - 'a';
        if (!node->children[index]) {
            return -1; /* Not a keyword */
        }
        node = node->children[index];
        word++;
    }
    /* Check if the node represents a keyword */
    return node->keyword_index >= 0 ? node->keyword_index : -1;
}

static ApexLexer_trie_node *create_trie_root() {
    ApexLexer_trie_node *root = create_trie_node();

    /* Insert all keywords */
    insert_word(root, "if", KEYWORD_IF);
    insert_word(root, "else", KEYWORD_ELSE);
    insert_word(root, "function", KEYWORD_FUNCTION);
    insert_word(root, "import", KEYWORD_IMPORT);
    insert_word(root, "int", KEYWORD_INT);
    insert_word(root, "float", KEYWORD_FLT);
    insert_word(root, "bool", KEYWORD_BOOL);
    insert_word(root, "true", KEYWORD_TRUE);
    insert_word(root, "false", KEYWORD_FALSE);
    insert_word(root, "string", KEYWORD_STR);
    insert_word(root, "void", KEYWORD_VOID);
    insert_word(root, "return", KEYWORD_RETURN);
    insert_word(root, "end", KEYWORD_END);
    return root;
}

static void reset_buf(ApexLexer *lexer) {
    lexer->buf.len = 0;
}

static void check_buf_size(ApexLexer *lexer) {
    if (lexer->buf.len >= lexer->buf.size) {
        lexer->buf.size += BUF_GROW_SIZE;
        lexer->buf.str = apex_realloc(lexer->buf.str, lexer->buf.size);
    }
}

static void set_token(ApexLexer *lexer, ApexLexer_token tk) {
    LEXER_TOKEN_INFO(lexer).token = tk;
    LEXER_TOKEN_INFO(lexer).lineno = lexer->lineno;
}

static char *finish_buf(ApexLexer *lexer) {
    char *buf = lexer->buf.str;

    buf[lexer->buf.len] = '\0';
    reset_buf(lexer); /* Restart a new buffer */
    return buf; /* Return the final result of the buffer. */
}

static void next_ch(ApexLexer *lexer) {
    lexer->c = getc(lexer->file);
}

static void save_and_next_ch(ApexLexer *lexer) {
    check_buf_size(lexer);
    lexer->buf.str[lexer->buf.len++] = lexer->c;
    next_ch(lexer);
}

static void get_eq(ApexLexer *lexer) {
    next_ch(lexer);
    if (lexer->c == '=') {
        next_ch(lexer);
        set_token(lexer, APEX_LEXER_TOKEN_EQUALS);
    } else {
        set_token(lexer, '=');
    }
    next_ch(lexer);
}

static void get_add(ApexLexer *lexer) {
    next_ch(lexer);
    switch (lexer->c) {
        case '+': /* Token is ++ so we add the increment token */
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_INC);
            break;

        case '=': /* Token is += so we add the assign and add token */
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_ASGN_ADD);
            break;

        default:
            set_token(lexer, '+');
            break;
    }
}

static void get_sub(ApexLexer *lexer) {
    next_ch(lexer);
    switch (lexer->c) {
        case '-': /* Token is -- so we add the decrement token */
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_DEC);
            break;

        case '=': /* Token is -= so we add the assign subtract token */
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_ASGN_SUB);
            break;

        default:
            set_token(lexer, '-');
            break;
    }
}

static void get_mul(ApexLexer *lexer) {
    next_ch(lexer);
    switch (lexer->c) {
        case '=':
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_ASGN_MUL);
            break;

        default:
            set_token(lexer, '*');
            break;
    }
}

static void get_div(ApexLexer *lexer) {
    next_ch(lexer);
    switch (lexer->c) {
        case '=':
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_ASGN_DIV);
            break;

        default:
            set_token(lexer, '/');
            break;
    }
}

static void get_not(ApexLexer *lexer) {
    next_ch(lexer);
    switch (lexer->c) {
        case '=':
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_NOT_EQUAL);
            break;

        default:
            set_token(lexer, '!');
            break;
    }
}

static void get_number(ApexLexer *lexer) {
    char *number;

    set_token(lexer, APEX_LEXER_TOKEN_INT); /* Assume int until found otherwise */
    for (;;) {
        if (lexer->c == '.' && LEXER_TOKEN_INFO(lexer).token != APEX_LEXER_TOKEN_FLT) {
            /* '.' found in number so it's a float */ 
            set_token(lexer, APEX_LEXER_TOKEN_FLT);
            save_and_next_ch(lexer);
        } else if (isdigit(lexer->c)) {
            save_and_next_ch(lexer);
        } else {
            break;
        }
    }
    number = finish_buf(lexer);

    if (LEXER_TOKEN_INFO(lexer).token == APEX_LEXER_TOKEN_INT) {
        LEXER_TOKEN_INFO_INT(lexer) = atoi(number);
    } else {
        LEXER_TOKEN_INFO_FLT(lexer) = atof(number);
    }
    next_ch(lexer);
}

static void get_str(ApexLexer *lexer) {
    next_ch(lexer); /* Skip the opening quote */
    while (lexer->c != '"' && lexer->c != EOF && lexer->c != '\n') {
        save_and_next_ch(lexer); 
        if (lexer->c == '\\') {
            save_and_next_ch(lexer); /* Include the escaped character */
        } 
    }
    if (lexer->c == '\n' || lexer->c == EOF) { /* Closing quote must be on the same line*/
        ApexError_syntax("incomplete string");
        return;
    }
    next_ch(lexer); /* Skip the closing quote */
    LEXER_TOKEN_INFO_STR(lexer) = finish_buf(lexer);
    set_token(lexer, APEX_LEXER_TOKEN_STR);
    next_ch(lexer);
}

static ApexLexer_token get_keyword_index_token(int keyword_index) {
    /* Get the associated token for each keyword */ 
    switch (keyword_index) {
    case KEYWORD_IF:
        return APEX_LEXER_TOKEN_IF;

    case KEYWORD_ELSE:
        return APEX_LEXER_TOKEN_ELSE;

    case KEYWORD_FUNCTION:
        return APEX_LEXER_TOKEN_DECL_FUNC;

    case KEYWORD_IMPORT:
        return APEX_LEXER_TOKEN_IMPORT;

    case KEYWORD_INT:
        return APEX_LEXER_TOKEN_DECL_INT;

    case KEYWORD_FLT:
        return APEX_LEXER_TOKEN_DECL_FLT;

    case KEYWORD_BOOL:
        return APEX_LEXER_TOKEN_DECL_BOOL;

    case KEYWORD_STR:
        return APEX_LEXER_TOKEN_DECL_STR;

    case KEYWORD_TRUE: 
        return APEX_LEXER_TOKEN_TRUE;

    case KEYWORD_FALSE:
        return APEX_LEXER_TOKEN_FALSE;

    case KEYWORD_VOID:
        return APEX_LEXER_TOKEN_VOID;

    case KEYWORD_RETURN:
        return APEX_LEXER_TOKEN_RETURN;

    case KEYWORD_END:
        return APEX_LEXER_TOKEN_END;
    }
    ApexError_syntax("No index for keyword %d", keyword_index);
    return keyword_index;
}

static void get_name_or_keyword(ApexLexer *lexer) {
    char *name;
    int keyword_index;
    while (isalnum(lexer->c) || lexer->c == '_') { 
        /* A name can only consist out of alphanumeric characters or a '_' */
        save_and_next_ch(lexer);
    }
    name = finish_buf(lexer); 
    keyword_index = search_keyword(lexer->trie_root, name); /* Search whether the string is a keyword */
    if (keyword_index != -1) {
        keyword_index = get_keyword_index_token(keyword_index);
        set_token(lexer, keyword_index); /* Token is a keyword */
    } else {
        set_token(lexer, APEX_LEXER_TOKEN_ID); /* keyword is not a token so it must be an identifier */
    }
    LEXER_TOKEN_INFO_STR(lexer) = name;
    reset_buf(lexer);
    next_ch(lexer);
}

static void init_next_token_info(ApexLexer *lexer) {
    if (lexer->c == EOF) {
        set_token(lexer, APEX_LEXER_TOKEN_EOF);
    } else {
        next_token(lexer);
    }
}

static void unexp(ApexLexer *lexer) {
    ApexError_syntax(
        "Unexpected character '%c' on line %d",
        lexer->c,
        lexer->lineno);
}

static void clear_spaces(ApexLexer *lexer) {
    char c = lexer->c;

    /* Ignore all space characters until another character is found */
    while (isspace(c)) {
        if (c == '\n') {
            lexer->lineno++;
            printf("1. lineno++\n");
        }
        next_ch(lexer); 
        c = lexer->c;
    }
}

static void next_token(ApexLexer *lexer) {
    char c = lexer->c;

    clear_spaces(lexer);

    if (isdigit(c)) {
        get_number(lexer);
        return;
    }
    
    if (isalpha(c)) { /* First character must always be an alphabetic character to be a name or keyword */
        get_name_or_keyword(lexer);
        return;
    }

    switch (c) {
    case '=':
        get_eq(lexer);
        break;

    case '+':
        get_add(lexer);
        break;

    case '-':
        get_sub(lexer);
        break;

    case '*':
        get_mul(lexer);
        break;

    case '/':
        get_div(lexer);
        break;

    case '"':
        get_str(lexer);
        break;

    case '(':
    case ')':
    case ':':
        next_ch(lexer);
        set_token(lexer, c);
        break;

    case '<':
        next_ch(lexer);
        if (lexer->c == '=') { /* Token is <= */
        next_ch(lexer);
        set_token(lexer, APEX_LEXER_TOKEN_LESS_EQUAL); 
    } else {
        set_token(lexer, APEX_LEXER_TOKEN_LESS);
    }
    break;

    case '>':
        next_ch(lexer);
        if (lexer->c == '=') { /* Token is >= */
            next_ch(lexer);
            set_token(lexer, APEX_LEXER_TOKEN_GREATER_EQUAL);
        } else {
            set_token(lexer, APEX_LEXER_TOKEN_GREATER);
        }
        break;

    case '&':
        next_ch(lexer);
        if (lexer->c != '&') { /* Token must be && */
            unexp(lexer);
            return;
        }
        next_ch(lexer);
        set_token(lexer, APEX_LEXER_TOKEN_AND);
        break;


    case '|':
        next_ch(lexer);
        if (lexer->c != '|') { /* Token must be || */
            unexp(lexer);
            return;
        }
        next_ch(lexer);
        set_token(lexer, APEX_LEXER_TOKEN_OR);
        break;
    case '\n':
        do {
            lexer->lineno++;
            printf("2. lineno++\n");
            next_ch(lexer);
        } while (lexer->c == '\n'); /* Skip consecutive newlines */
        set_token(lexer, APEX_LEXER_TOKEN_END_STMT);
        return;

    case '\0':
        set_token(lexer, APEX_LEXER_TOKEN_NULL_TERM);
        break;

    case EOF:
        set_token(lexer, APEX_LEXER_TOKEN_EOF);
        break;

    case '!':
        get_not(lexer);
        break;

    default:
        unexp(lexer);
    }
}

ApexLexer *ApexLexer_new(FILE *file) {
    ApexLexer *lexer = APEX_NEW(ApexLexer);

    lexer->lineno = 1;
    lexer->file = file;
    lexer->trie_root = create_trie_root();
    lexer->buf.str = apex_malloc(APEX_BUF_SIZE);
    lexer->buf.size = APEX_BUF_SIZE;
    lexer->buf.len = 0;

    reset_buf(lexer);
    lexer->c = getc(lexer->file);
    init_next_token_info(lexer);
    return lexer;
}

void ApexLexer_free(ApexLexer *lexer) {
    if (lexer) {
        destroy_trie(lexer->trie_root);
        apex_free(lexer->buf.str);
        apex_free(lexer);
    }
}

ApexLexer_token ApexLexer_next_token(ApexLexer *lexer) {
    lexer->cur = lexer->next;

    if (lexer->cur.token != APEX_LEXER_TOKEN_NULL_TERM) {
        next_token(lexer);
    }
    return lexer->cur.token;
}

char *ApexLexer_get_str(ApexLexer *lexer) {
    return STR_VAL(lexer);
}

int ApexLexer_get_int(ApexLexer *lexer) {
    return INT_VAL(lexer);
}

float ApexLexer_get_flt(ApexLexer *lexer) {
    return FLT_VAL(lexer);
}

bool ApexLexer_get_bool(ApexLexer *lexer) {
    return BOOL_VAL(lexer);
}

ApexLexer_token ApexLexer_get_token(ApexLexer *lexer) {
    return lexer->cur.token;
}

ApexLexer_token ApexLexer_peek(ApexLexer *lexer) {
    return lexer->next.token;
}

int ApexLexer_get_lineno(ApexLexer *lexer) {
    return lexer->cur.lineno;
}

ApexValue *ApexLexer_get_value(ApexLexer *lexer) {
    return &lexer->cur.value;
}