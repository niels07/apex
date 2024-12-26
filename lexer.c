#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "string.h"
#include "mem.h"
#include "error.h"

typedef struct {
    const char *str;
    size_t len;
} TokenStr;

/* String representations of tokens */
static TokenStr tk2str[] = {
    {"identifier", 10}, 
    {"int", 3}, 
    {"float", 5}, 
    {"string", 6}, 
    {"if", 2}, 
    {"elif", 4}, 
    {"else", 4},
    {"fn", 2},
    {"for", 3}, 
    {"while", 5},
    {"return", 6},
    {"+", 1}, 
    {"-", 1}, 
    {"*", 1}, 
    {"/", 1},
    {"++", 2}, 
    {"--", 2},
    {"=", 1}, 
    {"+=", 2}, 
    {"-=", 2}, 
    {"*=", 2}, 
    {"/=", 2},
    {"==", 2},
    {"!=", 2},
    {"<", 1},
    {">", 1}, 
    {"<=", 2}, 
    {">=", 2},
    {"!", 1},
    {"&", 1}, 
    {"&&", 2}, 
    {"|", 1}, 
    {"||", 2},
    {"(", 1},
    {")", 1},
    {"{", 1},
    {"}", 1},
    {",", 1}, 
    {";", 1},
    {"true", 4}, 
    {"false", 5},
    {"eof", 3}
};

/* Global strings used by the lexer. */
static const char *IF_STR, *ELIF_STR, *ELSE_STR,
    *FN_STR, 
    *FOR_STR, *WHILE_STR, *RETURN_STR, 
    *PLUS_STR, *MINUS_STR, *STAR_STR, *SLASH_STR, 
    *PLUS_PLUS_STR, *MINUS_MINUS_STR, 
    *EQUAL_STR, *PLUS_EQUAL_STR, *MINUS_EQUAL_STR, *STAR_EQUAL_STR, *SLASH_EQUAL_STR, 
    *EQUAL_EQUAL_STR, *NOT_EQUAL_STR, 
    *LESS_STR, *GREATER_STR, *LESS_EQUAL_STR, *GREATER_EQUAL_STR, 
    *NOT_STR,
    *AMP_STR, *AND_STR, *PIPE_STR, *OR_STR,
    *LPAREN_STR, *RPAREN_STR, 
    *LBRACE_STR, *RBRACE_STR, 
    *COMMA_STR, *SEMICOLON_STR, 
    *BREAK_STR, *CONTINUE_STR,
    *TRUE_STR, *FALSE_STR;

static int globals_initialized = 0;

/**
 * Initializes global strings used by the lexer.
 *
 * This function is called once, the first time that init_lexer is called.
 *
 * @see init_lexer
 */
static void init_lexer_globals() {
    IF_STR = new_string("if", 2);
    ELIF_STR = new_string("elif", 4);
    ELSE_STR = new_string("else", 4);
    FN_STR = new_string("fn", 2);
    FOR_STR = new_string("for", 3);
    WHILE_STR = new_string("while", 5);
    RETURN_STR = new_string("return", 6);
    PLUS_STR = new_string("+", 1);
    MINUS_STR = new_string("-", 1);
    STAR_STR = new_string("*", 1);
    SLASH_STR = new_string("/", 1);
    PLUS_PLUS_STR = new_string("++", 2);
    MINUS_MINUS_STR = new_string("--", 2);
    EQUAL_STR = new_string("=", 1);
    PLUS_EQUAL_STR = new_string("+=", 2);
    MINUS_EQUAL_STR = new_string("-=", 2);
    STAR_EQUAL_STR = new_string("*=", 2);
    SLASH_EQUAL_STR = new_string("/=", 2);
    EQUAL_EQUAL_STR = new_string("==", 2);
    NOT_EQUAL_STR = new_string("!=", 2);
    LESS_STR = new_string("<", 1);
    GREATER_STR = new_string(">", 1);
    LESS_EQUAL_STR = new_string("<=", 2);
    GREATER_EQUAL_STR = new_string(">=", 2);
    NOT_STR = new_string("!", 1);
    AMP_STR = new_string("&", 1);
    AND_STR = new_string("&&", 2);
    PIPE_STR = new_string("|", 1);
    OR_STR = new_string("||", 2);
    LPAREN_STR = new_string("(", 1);
    RPAREN_STR = new_string(")", 1);
    LBRACE_STR = new_string("{", 1);
    RBRACE_STR = new_string("}", 1);
    COMMA_STR = new_string(",", 1);
    SEMICOLON_STR = new_string(";", 1);
    CONTINUE_STR = new_string("continue", 8);
    BREAK_STR = new_string("break", 5);
    TRUE_STR = new_string("true", 4);
    FALSE_STR = new_string("false", 5);
};

/**
 * Retrieves a string representation of a token type.
 *
 * This function returns a newly allocated string representing the given
 * token type. The string is a copy of the corresponding string in the
 * tk2str array, and is therefore safe to free.
 *
 * @param type The token type to retrieve a string for.
 * @return A newly allocated string representing the given token type.
 */
char *get_token_str(TokenType type) {
    return new_string(tk2str[type].str, tk2str[type].len);
}

/**
 * Initializes a Lexer with the provided source code.
 *
 * This function sets up the Lexer structure by assigning the source code,
 * calculating its length, and initializing the current position and line
 * number. It also ensures that global lexer-related variables are initialized.
 *
 * @param lexer A pointer to the Lexer structure to initialize.
 * @param source A pointer to the source code string to be tokenized.
 */
void init_lexer(Lexer *lexer, const char *source) {
    if (!globals_initialized) {
        init_lexer_globals();
    }
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->line = 1;
}

/**
 * Creates a new Token structure with the given type, value, and line number.
 *
 * @param type The type of the token.
 * @param value The value of the token, or NULL if the token has no value.
 * @param lineno The line number of the token.
 * @return A pointer to the newly created Token structure.
 */
Token *create_token(TokenType type, char *value, int lineno) {
    Token *token = mem_alloc(sizeof(Token));
    token->type = type;
    token->value = value;
    token->lineno = lineno;
    return token;
}

/**
 * Frees the memory allocated for a token.
 *
 * This function releases the memory used by the given Token structure.
 *
 * @param token A pointer to the Token to be freed.
 */
void free_token(Token *token) {
    free(token);
}

/**
 * Peeks at the next character in the source code without advancing the lexer's position.
 *
 * This function is useful for looking ahead at the next character in the source code
 * without consuming it. It returns '\0' if the lexer has reached the end of the source
 * code.
 *
 * @param lexer A pointer to the Lexer structure to peek into.
 * @return The next character in the source code, or '\0' if the end of the source code
 * has been reached.
 */
static char peek(Lexer *lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    return lexer->source[lexer->position];
}

/**
 * Advances the lexer's position and returns the current character.
 *
 * This function increments the lexer's position to the next character
 * in the source code and returns the character at the current position.
 * If the end of the source code is reached, it returns '\0'.
 *
 * @param lexer A pointer to the Lexer structure to advance.
 * @return The current character in the source code, or '\0' if the end
 *         of the source code has been reached.
 */
static char advance(Lexer *lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    return lexer->source[lexer->position++];
}

/**
 * Skips over whitespace characters in the source code.
 *
 * This function advances the lexer's position past any whitespace
 * characters. If a newline character is encountered, the line
 * counter is incremented. The function stops when a non-whitespace
 * character or the end of the source is reached.
 *
 * @param lexer A pointer to the Lexer structure to update.
 */
static void skip_whitespace(Lexer *lexer) {
    while (isspace(peek(lexer))) {
        if (peek(lexer) == '\n') {
            lexer->line++;
        }
        advance(lexer);
    }
}

/**
 * Skips over any comments in the source code.
 *
 * This function advances the lexer's position past any characters
 * starting with '#'. If a '#' character is encountered, the lexer's
 * position is advanced until a newline character or the end of
 * the source code is reached.
 *
 * @param lexer A pointer to the Lexer structure to update.
 */
static void skip_comments(Lexer *lexer) {
    if (peek(lexer) == '#') {
        while (peek(lexer) != '\n' && peek(lexer) != '\0') {
            advance(lexer);
        }
    }
}

/**
 * Scans a number from the source code.
 *
 * This function scans a number from the source code, starting at the
 * current position of the lexer. The number can be an integer or a
 * float. If the number is an integer, the function will scan until
 * a non-digit character is encountered. If the number is a float, the
 * function will scan until a non-digit character is encountered after
 * the decimal point.
 *
 * @param lexer A pointer to the Lexer structure to update.
 * @return A pointer to a newly allocated Token structure with
 *         type TOKEN_INT or TOKEN_FLT, containing the number as a
 *         string, or NULL if the function encounters an error.
 */
static Token *scan_num(Lexer *lexer) {
    int start = lexer->position - 1;
    int len;
    char *num;
    TokenType token_type = TOKEN_INT;

    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    if (peek(lexer) == '.') {
        token_type = TOKEN_FLT;
        advance(lexer);
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    len = lexer->position - start;
    num = new_string(lexer->source + start, len);
    return create_token(token_type, num, lexer->line);
}

/**
 * Scans an identifier from the source code.
 *
 * This function scans an identifier from the source code, starting at the
 * current position of the lexer. The identifier is a string of alphanumeric
 * characters and underscores.
 *
 * @param lexer A pointer to the Lexer structure to update.
 * @return A pointer to a newly allocated Token structure with type
 *         TOKEN_IDENT or one of the keyword tokens, containing the
 *         identifier as a string, or NULL if the function encounters an
 *         error.
 */
static Token *scan_ident(Lexer *lexer) {
    int start = lexer->position - 1;
    int len;
    char *ident;

    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    len = lexer->position - start;
    ident = new_string(lexer->source + start, len);

    if (ident == IF_STR) {
        return create_token(TOKEN_IF, ident, lexer->line);
    } else if (ident == ELIF_STR) {
        return create_token(TOKEN_ELIF, ident, lexer->line);
    } else if (ident == ELSE_STR) {
        return create_token(TOKEN_ELSE, ident, lexer->line);
    } else if (ident == FN_STR) {
        return create_token(TOKEN_FN, ident, lexer->line);
    } else if (ident == FOR_STR) {
        return create_token(TOKEN_FOR, ident, lexer->line);
    } else if (ident == WHILE_STR) {
        return create_token(TOKEN_WHILE, ident, lexer->line);
    } else if (ident == TRUE_STR)  {
        return create_token(TOKEN_TRUE, ident, lexer->line);
    } else if (ident == FALSE_STR) {
        return create_token(TOKEN_FALSE, ident, lexer->line);
    } else if (ident == RETURN_STR) {
        return create_token(TOKEN_RETURN, ident, lexer->line);
    } else if (ident == BREAK_STR) {
        return create_token(TOKEN_BREAK, ident, lexer->line);
    } else if (ident == CONTINUE_STR) {
        return create_token(TOKEN_CONTINUE, ident, lexer->line);
    }
    return create_token(TOKEN_IDENT, ident, lexer->line);
}

/**
 * Scans a string from the source code.
 *
 * This function scans a string from the source code, starting at the
 * current position of the lexer. The string is a sequence of characters
 * enclosed in double quotes, and may contain escaped characters.
 *
 * @param lexer A pointer to the Lexer structure to update.
 * @return A pointer to a newly allocated Token structure with type
 *         TOKEN_STR, containing the string as a string, or NULL if the
 *         function encounters an error.
 */
static Token *scan_str(Lexer *lexer) {
    int start = lexer->position - 1;
    int len;
    char *str;

    if (lexer->source[start] != '"') {    
        while (peek(lexer) != '"') {
            advance(lexer);
            if (peek(lexer) == '\\') {
                advance(lexer);
            }
        }
        advance(lexer);
    }
    
    len = lexer->position - start - 1;
    str = new_string(lexer->source + start, len);
    return create_token(TOKEN_STR, str, lexer->line);
}

/**
 * Retrieves the next token from the source code.
 *
 * This function processes the input source code and generates a token
 * representing the current lexeme. It handles different types of tokens
 * including identifiers, numbers (integers and floats), strings, operators,
 * and punctuation. The function skips whitespace and comments before
 * processing the next meaningful token. If the end of the source code is
 * reached, a TOKEN_EOF is returned. If an unexpected character is
 * encountered, a syntax error is reported.
 *
 * @param lexer A pointer to the Lexer structure containing the source code
 *              and current position.
 * @return A pointer to a newly allocated Token structure, or NULL if a
 *         syntax error is encountered.
 */
Token *get_next_token(Lexer *lexer) {
    char c;
    skip_whitespace(lexer);
    skip_comments(lexer);

    if (lexer->position >= lexer->length) {
        return create_token(TOKEN_EOF, new_string("EOF", 3), lexer->line);
    }

    c = advance(lexer);

    if (isalpha(c)) {
        return scan_ident(lexer);
    }
    if (isdigit(c)) {
        return scan_num(lexer);
    }

    switch (c) {
    case '"':
        advance(lexer);
        return scan_str(lexer);
    case '=':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_EQUAL_EQUAL, 
                (char *)EQUAL_EQUAL_STR, 
                lexer->line);
        } 
        return create_token(
            TOKEN_EQUAL,
            (char *)EQUAL_STR,
            lexer->line);      
    case '+':
        if (peek(lexer) == '+') {
            advance(lexer);
            return create_token(
                TOKEN_PLUS_PLUS,
                (char *)PLUS_PLUS_STR,
                lexer->line);
        } else if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_PLUS_EQUAL, 
                (char *)PLUS_EQUAL_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_PLUS,
            (char *)PLUS_STR,
            lexer->line);
    case '-':
        if (peek(lexer) == '-') {
            advance(lexer);
            return create_token(
                TOKEN_MINUS_MINUS,
                (char *)MINUS_MINUS_STR,
                lexer->line);
        } else if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_MINUS_EQUAL,
                (char *)MINUS_EQUAL_STR,
                lexer->line);
        }
        return create_token(
            TOKEN_MINUS, 
            (char *)MINUS_STR, 
            lexer->line);
    case '*': 
        return create_token(
            TOKEN_STAR, 
            (char *)STAR_STR, 
            lexer->line);
    case '/': 
        return create_token(
            TOKEN_SLASH, 
            (char *)SLASH_STR, 
            lexer->line);
    case '(': 
        return create_token(
            TOKEN_LPAREN, 
            (char *)LPAREN_STR, 
            lexer->line);
    case ')': 
        return create_token(
            TOKEN_RPAREN, 
            (char *)RPAREN_STR, 
            lexer->line);
    case '{': 
        return create_token(
            TOKEN_LBRACE, 
            (char *)LBRACE_STR, 
            lexer->line);
    case '}': 
        return create_token(
            TOKEN_RBRACE, 
            (char *)RBRACE_STR, 
            lexer->line);
    case ',': 
        return create_token(
            TOKEN_COMMA, 
            (char *)COMMA_STR, 
            lexer->line);
    case ';': 
        return create_token(
            TOKEN_SEMICOLON, 
            (char *)SEMICOLON_STR, 
            lexer->line);
    case '<':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_LESS_EQUAL, 
                (char *)LESS_EQUAL_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_LESS, 
            (char *)LESS_STR, 
            lexer->line);
    case '>':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_GREATER_EQUAL, 
                (char *)GREATER_EQUAL_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_GREATER, 
            (char *)GREATER_STR, 
            lexer->line);    
    case '!': 
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(
                TOKEN_NOT_EQUAL, 
                (char *)NOT_EQUAL_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_NOT, 
            (char *)NOT_STR, 
            lexer->line);
    case '&':
        if (peek(lexer) == '&') {
            advance(lexer);
            return create_token(
                TOKEN_AND, 
                (char *)AND_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_AMP, 
            (char *)AMP_STR, 
            lexer->line);
    case '|':
        if (peek(lexer) == '|') {
            advance(lexer);
            return create_token(
                TOKEN_OR, 
                (char *)OR_STR, 
                lexer->line);
        }
        return create_token(
            TOKEN_PIPE, 
            (char *)PIPE_STR, 
            lexer->line);
    }

    apexErr_syntax(lexer->line, "Unexpected character: %c", c);
    return NULL;
}
