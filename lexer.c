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
    {"double", 6}, 
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
    {"break", 5},
    {"continue", 8},
    {"(", 1},
    {")", 1},
    {"{", 1},
    {"}", 1},
    {"[", 1},
    {"]", 1},
    {",", 1}, 
    {";", 1},
    {"true", 4}, 
    {"false", 5},
    {"=>", 2}, 
    {"include", 7},
    {".", 1},
    {"eof", 3}
};

/* Global strings used by the lexer. */
static ApexString *IF_STR, *ELIF_STR, *ELSE_STR,
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
    *LBRACKET_STR, *RBRACKET_STR,
    *COMMA_STR, *SEMICOLON_STR, 
    *BREAK_STR, *CONTINUE_STR,
    *TRUE_STR, *FALSE_STR,
    *INCLUDE_STR,
    *ARROW_STR,
    *DOT_STR, *COLON_STR;

static int globals_initialized = 0;

/**
 * Initializes global strings used by the lexer.
 *
 * This function is called once, the first time that init_lexer is called.
 *
 * @see init_lexer
 */
static void init_lexer_globals() {
    IF_STR = apexStr_new("if", 2);
    ELIF_STR = apexStr_new("elif", 4);
    ELSE_STR = apexStr_new("else", 4);
    FN_STR = apexStr_new("fn", 2);
    FOR_STR = apexStr_new("for", 3);
    WHILE_STR = apexStr_new("while", 5);
    RETURN_STR = apexStr_new("return", 6);
    PLUS_STR = apexStr_new("+", 1);
    MINUS_STR = apexStr_new("-", 1);
    STAR_STR = apexStr_new("*", 1);
    SLASH_STR = apexStr_new("/", 1);
    PLUS_PLUS_STR = apexStr_new("++", 2);
    MINUS_MINUS_STR = apexStr_new("--", 2);
    EQUAL_STR = apexStr_new("=", 1);
    PLUS_EQUAL_STR = apexStr_new("+=", 2);
    MINUS_EQUAL_STR = apexStr_new("-=", 2);
    STAR_EQUAL_STR = apexStr_new("*=", 2);
    SLASH_EQUAL_STR = apexStr_new("/=", 2);
    EQUAL_EQUAL_STR = apexStr_new("==", 2);
    NOT_EQUAL_STR = apexStr_new("!=", 2);
    LESS_STR = apexStr_new("<", 1);
    GREATER_STR = apexStr_new(">", 1);
    LESS_EQUAL_STR = apexStr_new("<=", 2);
    GREATER_EQUAL_STR = apexStr_new(">=", 2);
    NOT_STR = apexStr_new("!", 1);
    AMP_STR = apexStr_new("&", 1);
    AND_STR = apexStr_new("&&", 2);
    PIPE_STR = apexStr_new("|", 1);
    OR_STR = apexStr_new("||", 2);
    LPAREN_STR = apexStr_new("(", 1);
    RPAREN_STR = apexStr_new(")", 1);
    LBRACE_STR = apexStr_new("{", 1);
    RBRACE_STR = apexStr_new("}", 1);
    LBRACKET_STR = apexStr_new("[", 1);
    RBRACKET_STR = apexStr_new("]", 1);
    COMMA_STR = apexStr_new(",", 1);
    SEMICOLON_STR = apexStr_new(";", 1);
    CONTINUE_STR = apexStr_new("continue", 8);
    BREAK_STR = apexStr_new("break", 5);
    TRUE_STR = apexStr_new("true", 4);
    FALSE_STR = apexStr_new("false", 5);
    ARROW_STR = apexStr_new("=>", 2);
    INCLUDE_STR = apexStr_new("include", 7);
    DOT_STR = apexStr_new(".", 1);
    COLON_STR = apexStr_new(":", 1);
    globals_initialized = 1;
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
ApexString *get_token_str(TokenType type) {
    return apexStr_new(tk2str[type].str, tk2str[type].len);
}


/**
 * Initializes a Lexer with the given source code and filename.
 *
 * This function sets up the Lexer by assigning it the given source code
 * and filename, and initializing its position to 0 and its line number to
 * 1.
 *
 * @param lexer A pointer to the Lexer structure to initialize.
 * @param filename The name of the file being parsed, or NULL if the source
 *                 code is not from a file.
 * @param source The source code to parse.
 */
void init_lexer(Lexer *lexer, const char *filename, const char *source) {
    if (!globals_initialized) {
        init_lexer_globals();
    }
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->srcloc.lineno = 1;
    lexer->srcloc.filename = filename;
}

/**
 * Creates a new Token with the specified type and value.
 *
 * This function allocates memory for a new Token structure and
 * initializes its fields with the provided type and value, along
 * with the source location from the given Lexer.
 *
 * @param lexer A pointer to the Lexer structure providing the source location.
 * @param type The type of the token to be created.
 * @param value The value of the token as a string.
 * @return A pointer to the newly created Token structure.
 */
Token *create_token(Lexer *lexer, TokenType type, ApexString *str) {
    Token *token = apexMem_alloc(sizeof(Token));
    token->type = type;
    token->str = str;
    token->srcloc = lexer->srcloc;;
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
            lexer->srcloc.lineno++;
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
 *         type TOKEN_INT or TOKEN_DBL, containing the number as a
 *         string, or NULL if the function encounters an error.
 */
static Token *scan_num(Lexer *lexer) {
    int start = lexer->position - 1;
    TokenType token_type = TOKEN_INT;

    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    if (peek(lexer) == '.') {
        token_type = TOKEN_DBL;
        advance(lexer);
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    int len = lexer->position - start;
    ApexString *num = apexStr_new(lexer->source + start, len);
    return create_token(lexer, token_type, num);
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

    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    int len = lexer->position - start;
    ApexString *ident = apexStr_new(lexer->source + start, len);

    if (ident == IF_STR) {
        return create_token(lexer, TOKEN_IF, ident);
    } else if (ident == ELIF_STR) {
        return create_token(lexer, TOKEN_ELIF, ident);
    } else if (ident == ELSE_STR) {
        return create_token(lexer, TOKEN_ELSE, ident);
    } else if (ident == FN_STR) {
        return create_token(lexer, TOKEN_FN, ident);
    } else if (ident == FOR_STR) {
        return create_token(lexer, TOKEN_FOR, ident);
    } else if (ident == WHILE_STR) {
        return create_token(lexer, TOKEN_WHILE, ident);
    } else if (ident == TRUE_STR)  {
        return create_token(lexer, TOKEN_TRUE, ident);
    } else if (ident == FALSE_STR) {
        return create_token(lexer, TOKEN_FALSE, ident);
    } else if (ident == RETURN_STR) {
        return create_token(lexer, TOKEN_RETURN, ident);
    } else if (ident == BREAK_STR) {
        return create_token(lexer, TOKEN_BREAK, ident);
    } else if (ident == CONTINUE_STR) {
        return create_token(lexer, TOKEN_CONTINUE, ident);
    } else if (ident == INCLUDE_STR) {
        return create_token(lexer, TOKEN_INCLUDE, ident);
    }
    return create_token(lexer, TOKEN_IDENT, ident);
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
    int start = lexer->position;
    char *buffer = apexMem_alloc(lexer->length - start);
    int i = 0;

    while (peek(lexer) != '"' && peek(lexer) != '\0') { 
        char c = advance(lexer);
        if (c == '\\') { // Handle escape sequences.
            char next = advance(lexer);
            switch (next) {
            case 'n':
                buffer[i++] = '\n';
                break;
            case 't':
                buffer[i++] = '\t';
                break;
            case 'r':
                buffer[i++] = '\r';
                break;
            case '\\':
                buffer[i++] = '\\';
                break;
            case '"':
                buffer[i++] = '"';
                break;
            default:                
                buffer[i++] = next;
                break;
            }
        } else {
            buffer[i++] = c;
        }
    }

    if (peek(lexer) == '"') {
        advance(lexer); // Consume closing quote.
    } else {
        apexErr_syntax(lexer, "Unterminated string literal.");
    }
    buffer[i] = '\0';
    
    ApexString *str = apexStr_save(buffer, i);
    return create_token(lexer, TOKEN_STR, str);
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
    skip_whitespace(lexer);
    skip_comments(lexer);

    if (lexer->position >= lexer->length) {
        return create_token(lexer, TOKEN_EOF, apexStr_new("EOF", 3));
    }

    char c = advance(lexer);

    if (isalpha(c)) {
        return scan_ident(lexer);
    }
    if (isdigit(c)) {
        return scan_num(lexer);
    }

    switch (c) {
    case '"':
        
        return scan_str(lexer);
    case '=':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_EQUAL_EQUAL,  EQUAL_EQUAL_STR);
        } else if (peek(lexer) == '>') {
            advance(lexer);
            return create_token(lexer, TOKEN_ARROW, ARROW_STR);
        }
        return create_token(lexer, TOKEN_EQUAL, EQUAL_STR);      
    case '+':
        if (peek(lexer) == '+') {
            advance(lexer);
            return create_token(lexer, TOKEN_PLUS_PLUS, PLUS_PLUS_STR);
        } else if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_PLUS_EQUAL, PLUS_EQUAL_STR);
        }
        return create_token(lexer, TOKEN_PLUS, PLUS_STR);
    case '-':
        if (peek(lexer) == '-') {
            advance(lexer);
            return create_token(lexer, TOKEN_MINUS_MINUS, MINUS_MINUS_STR);
        } else if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_MINUS_EQUAL, MINUS_EQUAL_STR);
        }
        return create_token(lexer, TOKEN_MINUS, MINUS_STR);
    case '*': 
        return create_token(lexer, TOKEN_STAR, STAR_STR);
    case '/': 
        return create_token(lexer, TOKEN_SLASH, SLASH_STR);
    case '(': 
        return create_token(lexer, TOKEN_LPAREN, LPAREN_STR);
    case ')': 
        return create_token(lexer, TOKEN_RPAREN, RPAREN_STR);
    case '{': 
        return create_token(lexer, TOKEN_LBRACE, LBRACE_STR);
    case '}': 
        return create_token(lexer, TOKEN_RBRACE, RBRACE_STR);
    case '[':
        return create_token(lexer, TOKEN_LBRACKET, LBRACKET_STR);
    case ']':
        return create_token(lexer, TOKEN_RBRACKET, RBRACKET_STR);    
    case ',': 
        return create_token(lexer, TOKEN_COMMA, COMMA_STR);
    case ';': 
        return create_token(lexer, TOKEN_SEMICOLON, SEMICOLON_STR);
    case '<':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_LESS_EQUAL, LESS_EQUAL_STR);
        }
        return create_token(lexer, TOKEN_LESS, LESS_STR);
    case '>':
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_GREATER_EQUAL, GREATER_EQUAL_STR);
        }
        return create_token(lexer, TOKEN_GREATER, GREATER_STR);    
    case '!': 
        if (peek(lexer) == '=') {
            advance(lexer);
            return create_token(lexer, TOKEN_NOT_EQUAL, NOT_EQUAL_STR);
        }
        return create_token(lexer, TOKEN_NOT, NOT_STR);
    case '&':
        if (peek(lexer) == '&') {
            advance(lexer);
            return create_token(lexer, TOKEN_AND, AND_STR);
        }
        return create_token(lexer, TOKEN_AMP, AMP_STR);
    case '|':
        if (peek(lexer) == '|') {
            advance(lexer);
            return create_token(lexer, TOKEN_OR, OR_STR);
        }
        return create_token(lexer, TOKEN_PIPE, PIPE_STR);
    case '.':
        return create_token(lexer, TOKEN_DOT, DOT_STR);
    case ':':
        return create_token(lexer, TOKEN_COLON, COLON_STR);
    }

    apexErr_syntax(lexer, "Unexpected character: '%c'", c);
    return NULL;
}
