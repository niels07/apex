#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "error.h"
#include "string.h"

static AST *parse_logical(Parser *parser);
static AST *parse_expression(Parser *parser);
static AST *parse_function_call(Parser *parser);
static AST *parse_primary(Parser *parser);
static AST *parse_unary(Parser *parser);
static AST *parse_factor(Parser *parser) ;
static AST *parse_bitwise(Parser *parser);
static AST *parse_term(Parser *parser);
static AST *parse_comparison(Parser *parser);
static AST *parse_assignment(Parser *parser);
static AST *parse_block(Parser *parser);
static AST *parse_if_statement(Parser *parser);
static AST *parse_while_statement(Parser *parser);
static AST *parse_for_statement(Parser *parser);
static AST *parse_return_statement(Parser *parser);
static AST *parse_function_declaration(Parser *parser);
static AST *parse_statement(Parser *parser);
static void consume(Parser *parser, TokenType type);

/**
 * Initializes a Parser with the provided Lexer.
 *
 * This function sets up the Parser by assigning it the given Lexer and
 * fetching the first token from the Lexer to initialize the current token.
 *
 * @param parser A pointer to the Parser structure to initialize.
 * @param lexer A pointer to the Lexer structure to be used by the Parser.
 */
void init_parser(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current_token = get_next_token(lexer);
}


/**
 * Peeks ahead in the token stream by a specified number of tokens.
 *
 * This function creates a backup of the current lexer state and retrieves
 * the token that is 'count' tokens ahead in the input stream without
 * modifying the parser's current token state. The retrieved token is returned,
 * allowing the parser to look ahead in the token stream for decision making.
 *
 * @param parser A pointer to the Parser containing the lexer and tokens.
 * @param count The number of tokens to look ahead in the token stream.
 * @return A Token representing the token 'count' positions ahead in the stream.
 */
static Token peek_token(Parser *parser, int count) {
    Lexer backup_lexer = *parser->lexer;
    Token *next_token;
    Token next;
    while (count--) {
        next_token = get_next_token(&backup_lexer);
        next = *next_token;
        free_token(next_token);
    }
    return next;
}

/**
 * Consumes a token from the lexer and updates the current token.
 *
 * This function verifies that the current token matches the provided
 * type. If the types match, the current token is freed and the next
 * token is retrieved from the lexer. If the types do not match, a
 * syntax error is reported and the parser is synchronized if the
 * sync_on_err flag is set.
 *
 * @param parser A pointer to the Parser containing the lexer and tokens.
 * @param type The type of token to consume.
 * @param sync_on_err Whether to synchronize the parser on error.
 */
static void consume(Parser *parser, TokenType type) {
    if (parser->current_token->type == type) {
        free_token(parser->current_token);
        parser->current_token = get_next_token(parser->lexer);
    } else {
        apexErr_syntax(parser->lexer, 
            "expected '%s' but found '%s'", 
            get_token_str(type)->value, 
            parser->current_token->str->value);
    }
}

/**
 * Appends the given AST node to the end of the given AST list.
 *
 * This function takes an existing AST list and a new AST node, and appends
 * the new node to the end of the list. If the given list is NULL, the new
 * node is returned as the new list.
 *
 * @param list The AST list to append to.
 * @param new_node The new AST node to append.
 *
 * @return The updated AST list with the new node appended.
 */
static AST *append_ast(AST *list, AST *new_node) {
    if (!list) {
        return new_node;
    }

    AST *current = list;
    while (current->right) {
        current = current->right;
    }
    current->right = new_node;
    return list;
}

/**
 * Parses an equality expression from the input tokens.
 *
 * This function parses an equality expression as a series of comparisons
 * joined by equality operators. The left operand is the result of the
 * previous comparison, and the right operand is the result of the next
 * comparison. If the current token is not an equality operator, the
 * function returns the result of the comparison.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed equality expression.
 */
static AST *parse_equality(Parser *parser) {
    AST *left = parse_comparison(parser); 

    while (parser->current_token->type == TOKEN_EQUAL_EQUAL || 
           parser->current_token->type == TOKEN_NOT_EQUAL) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_comparison(parser);
        left = CREATE_AST_STR(
            AST_BINARY_EXPR, left, right,
            get_token_str(operator), 
            parser->current_token->srcloc);
    }

    return left;
}

/**
 * Parses a logical expression from the input tokens.
 *
 * This function parses a logical expression as a series of equality
 * comparisons joined by logical operators. The left operand is the result
 * of the previous comparison, and the right operand is the result of the
 * next comparison.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed logical expression.
 */
static AST *parse_logical(Parser *parser) {
    AST *left = parse_equality(parser);

    while (parser->current_token->type == TOKEN_AND || 
           parser->current_token->type == TOKEN_OR) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_equality(parser);
        left = CREATE_AST_STR(
            AST_LOGICAL_EXPR, left, right, 
            get_token_str(operator),
            parser->current_token->srcloc);
        }
    return left;
}

/**
 * Parses an expression from the input tokens.
 *
 * This function is a wrapper around parse_logical, and will parse a logical
 * expression (i.e. a series of equality comparisons joined by logical operators).
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed expression, or an
 *         error node if a syntax error is encountered.
 */
static AST *parse_expression(Parser *parser) {
    return parse_logical(parser);
}

/**
 * Parses the argument list of a function call from the input tokens.
 *
 * This function assumes that the current token is the start of the argument
 * list, and will parse the arguments until it encounters a closing parenthesis.
 * The arguments are parsed as expressions and added to a linked list of AST
 * nodes. If a comma or closing parenthesis is not found in the argument list,
 * a syntax error is reported, and the current token is consumed to recover.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed argument list, or
 *         an error node if a syntax error is encountered.
 */
static AST *parse_fn_args(Parser *parser) {
    AST *arguments = NULL;
    while (parser->current_token->type != TOKEN_RPAREN) {
        AST *argument = parse_expression(parser);

        arguments = CREATE_AST_ZERO(
            AST_ARGUMENT_LIST, arguments, argument,
            parser->current_token->srcloc);
        
        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        } else if (parser->current_token->type != TOKEN_RPAREN) {
            apexErr_syntax(parser->lexer, "expected ',' or ')' in argument list.");
            consume(parser, parser->current_token->type);
        }
    }
    consume(parser, TOKEN_RPAREN);
    return arguments;
}

/**
 * Parses a function call from the input tokens.
 *
 * This function assumes that the current token is the start of a function call,
 * and will parse the function name, arguments, and closing parenthesis. If the
 * function name is not an identifier, a syntax error is reported and an error
 * node is returned. The arguments are parsed as expressions and added to a
 * linked list of AST nodes. If a comma or closing parenthesis is not found in
 * the argument list, a syntax error is reported, and the current token is
 * consumed to recover.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed function call, or
 *         an error node if a syntax error is encountered.
 */
static AST *parse_function_call(Parser *parser) {
    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer, "expected function name before '('.");
    }

    AST *function_name = CREATE_AST_STR(
        AST_VAR, NULL, NULL, 
        parser->current_token->str,
        parser->current_token->srcloc);

    consume(parser, TOKEN_IDENT);
    consume(parser, TOKEN_LPAREN);

    AST *arguments = parse_fn_args(parser);

    return CREATE_AST_ZERO(
        AST_FN_CALL, function_name, arguments,
        parser->current_token->srcloc);
}

/**
 * Parses a member access or member function call from the input tokens.
 *
 * This function assumes that the current token is the start of a member access
 * or member function call, and will parse the member name, and optionally the
 * arguments if it is a function call. If the member name is not an identifier,
 * a syntax error is reported.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @param node A pointer to an AST node representing the base of the member
 *             access.
 * @return A pointer to an AST node representing the parsed member access or
 *         member function call, or an error node if a syntax error is
 *         encountered.
 */
static AST *parse_member(Parser *parser, AST *node) {
    while (parser->current_token->type == TOKEN_DOT) {
        consume(parser, TOKEN_DOT);
        if (parser->current_token->type != TOKEN_IDENT) {
            apexErr_syntax(parser->lexer, "expected member name after '.'");
        }
        AST *member = CREATE_AST_STR(
            AST_VAR, NULL, NULL, 
            parser->current_token->str, 
            parser->current_token->srcloc);
        consume(parser, TOKEN_IDENT);

        node = CREATE_AST_ZERO(
            AST_MEMBER_ACCESS, node, member, 
            parser->current_token->srcloc);
        
        // Check if the member is a function call
        if (parser->current_token->type == TOKEN_LPAREN) {
            // Member function call: e.g., obj.new(...)
            consume(parser, TOKEN_LPAREN);
            AST *arguments = parse_fn_args(parser);       

            if (member->value.strval == apexStr_new("new", 3)) {
                if (node->type != AST_VAR && node->type != AST_MEMBER_ACCESS) {
                    apexErr_syntax(parser->lexer, "'new' can only be used in object contexts");
                }
                // Handle obj.new(...) as object creation
                node = CREATE_AST_ZERO(
                    AST_NEW, node->left, arguments,
                    parser->current_token->srcloc);
            } else {
                // General member function call
                node = CREATE_AST_ZERO(
                    AST_FN_CALL, node, arguments, 
                    parser->current_token->srcloc);
            }        
        }
    }
    return node;
}

/**
 * Parses an identifier from the input tokens.
 *
 * This function handles parsing of identifiers as either simple variable
 * references, member access, or array access. It also handles
 * post-increment and post-decrement operators.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed identifier.
 */
static AST *parse_ident(Parser *parser) {
    if (peek_token(parser, 1).type == TOKEN_LPAREN) {
        return parse_function_call(parser);
    }
    AST *node = CREATE_AST_STR(
        AST_VAR, NULL, NULL, 
        parser->current_token->str, 
        parser->current_token->srcloc);
    consume(parser, TOKEN_IDENT);
    while (parser->current_token->type == TOKEN_DOT) {
        node = parse_member(parser, node);
    }
    // Check if the identifier is an array access
    while (parser->current_token->type == TOKEN_LBRACKET) {
        consume(parser, TOKEN_LBRACKET);
        AST *index = parse_expression(parser);
        consume(parser, TOKEN_RBRACKET);

        node = CREATE_AST_ZERO(
            AST_ARRAY_ACCESS, node, index, 
            parser->current_token->srcloc);
    }
    // Check for unary expression (++, --)
    if (parser->current_token->type == TOKEN_PLUS_PLUS ||
        parser->current_token->type == TOKEN_MINUS_MINUS) {
        TokenType operator = parser->current_token->type;
        consume(parser, operator);
        node = CREATE_AST_STR(
            AST_UNARY_EXPR, node, NULL, 
            get_token_str(operator), 
            parser->current_token->srcloc);
    }

    return node;
}

/**
 * Parses an array literal from the input tokens.
 *
 * This function handles parsing of array literals, which may contain
 * expressions, key-value pairs, or both. The array literal is parsed
 * into a linked list of AST nodes, with each node of type AST_ARRAY.
 * If a syntax error is encountered, an error node is returned, and
 * the current token is consumed to recover.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed array literal, or
 *         an error node if a syntax error is encountered.
 */
static AST *parse_array(Parser *parser) {
    AST *node = CREATE_AST_ZERO(
        AST_ARRAY, NULL, NULL,
        parser->current_token->srcloc);      

    consume(parser, TOKEN_LBRACKET);

    if (parser->current_token->type == TOKEN_RBRACKET) {
        consume(parser, TOKEN_RBRACKET);
        return node;
    }

    while (parser->current_token->type != TOKEN_RBRACKET) {
        // Check if the array literal contains a key-value pair
        if (peek_token(parser, 1).type == TOKEN_ARROW) {
            AST *key = parse_expression(parser);
            
            consume(parser, TOKEN_ARROW);
            AST *value = parse_expression(parser);
            AST *key_value_pair = CREATE_AST_ZERO(
                AST_KEY_VALUE_PAIR, key, value,
                parser->current_token->srcloc);

            node->right = append_ast(node->right, key_value_pair);
        } else { 
            // Parse an expression as an array element
            AST *value = parse_expression(parser);
            node->right = append_ast(node->right, value);
        }

        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        } else if (parser->current_token->type != TOKEN_RBRACKET) {
            apexErr_syntax(parser->lexer, "expected ',' or ']' in array literal");
            consume(parser, parser->current_token->type);
        }
    }
    
    consume(parser, TOKEN_RBRACKET);
    return node;
}

/**
 * Parses a primary expression from the input tokens.
 *
 * This function handles the parsing of primary expressions, which include
 * integer literals, float literals, string literals, boolean literals
 * (true/false), identifiers, and parenthesized expressions. If the current
 * token is an identifier followed by a left parenthesis, it is parsed as
 * a function call.
 *
 * If the current token does not match any expected primary expression type,
 * a syntax error is reported, and an error node is returned.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed primary expression,
 *         or an error node if a syntax error is encountered.
 */
static AST *parse_primary(Parser *parser) {
    AST *node = NULL;

    switch (parser->current_token->type) {
    case TOKEN_INT:
        node = CREATE_AST_STR(
            AST_INT, NULL, NULL, 
            parser->current_token->str, 
            parser->current_token->srcloc);
        consume(parser, TOKEN_INT);
        return node;
    
    case TOKEN_DBL:
        node = CREATE_AST_STR(
            AST_DBL, NULL, NULL, 
            parser->current_token->str, 
            parser->current_token->srcloc);
        consume(parser, TOKEN_DBL);
        return node;

    case TOKEN_STR:
        node = CREATE_AST_STR(
            AST_STR, NULL, NULL, 
            parser->current_token->str, 
            parser->current_token->srcloc);
        consume(parser, TOKEN_STR);
        return node;

    case TOKEN_TRUE: 
    case TOKEN_FALSE:
        node = CREATE_AST_STR(
            AST_BOOL, NULL, NULL, 
            parser->current_token->str, 
            parser->current_token->srcloc);
        consume(parser, parser->current_token->type);
        return node;

    case TOKEN_IDENT:
        return parse_ident(parser);
    
    case TOKEN_LPAREN:
        consume(parser, TOKEN_LPAREN);
        node = parse_expression(parser);
        consume(parser, TOKEN_RPAREN);
        return node;

    case TOKEN_LBRACKET: 
        return parse_array(parser);   

    default:
        apexErr_syntax(parser->lexer, 
            "unexpected '%s', expecting %s, %s, %s, %s, %s, %s or %s", 
            parser->current_token->str->value,
            get_token_str(TOKEN_INT)->value, 
            get_token_str(TOKEN_DBL)->value, 
            get_token_str(TOKEN_STR)->value,
            get_token_str(TOKEN_TRUE)->value, 
            get_token_str(TOKEN_FALSE)->value, 
            get_token_str(TOKEN_IDENT)->value, 
            get_token_str(TOKEN_LPAREN)->value);
        return NULL;
    }
}

/**
 * Parses a unary operator from the input tokens.
 *
 * This function handles parsing of unary operators, specifically the
 * prefix and postfix increment/decrement operators ('++' and '--').
 * If the current token is not a unary operator, the function returns
 * the child node unchanged.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @param child The AST node representing the expression to be modified by the
 *              unary operator.
 * @param is_postfix A boolean indicating whether the unary operator is a
 *                   postfix operator (i.e. whether it appears after the
 *                   expression it modifies).
 * @return A pointer to an AST node representing the parsed unary expression.
 */
static AST *parse_unary_operator(Parser *parser, AST *child, bool is_postfix) {
    const char *op_str;

    if (parser->current_token->type == TOKEN_PLUS_PLUS) {
        op_str = "++";
    } else if (parser->current_token->type == TOKEN_MINUS_MINUS) {
        op_str = "--";
    } else {
        return child;
    }

    consume(parser, parser->current_token->type);
    return CREATE_AST_STR(
        AST_UNARY_EXPR, is_postfix ? child : NULL, 
        is_postfix ? NULL : child, 
        apexStr_new(op_str, 2), 
        parser->current_token->srcloc);
}

/**
 * Parses a unary expression from the input tokens.
 *
 * This function recognizes and parses unary expressions, which include
 * increment (++) and decrement (--) operations applied to primary expressions.
 * It creates AST nodes for these unary operations and returns them. If no
 * unary operator is present, the function defaults to parsing a primary
 * expression.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed unary expression
 *         or primary expression if no unary operator is present.
 */
static AST *parse_unary(Parser *parser) {
    AST *node = NULL;

    if (parser->current_token->type == TOKEN_MINUS ||
        parser->current_token->type == TOKEN_PLUS ||
        parser->current_token->type == TOKEN_NOT) {
        TokenType operator = parser->current_token->type;
        consume(parser, operator);
        node = parse_primary(parser);
        return CREATE_AST_STR(
            AST_UNARY_EXPR, NULL, node, 
            get_token_str(operator), 
            parser->current_token->srcloc);
    }

    if (parser->current_token->type == TOKEN_PLUS_PLUS || 
        parser->current_token->type == TOKEN_MINUS_MINUS) {
        TokenType operator = parser->current_token->type;
        consume(parser, operator);
        node = parse_primary(parser);
        return CREATE_AST_STR(
            AST_UNARY_EXPR, NULL, node, 
            get_token_str(operator), 
            parser->current_token->srcloc);
    }
    node = parse_primary(parser);

     if (parser->current_token->type == TOKEN_PLUS_PLUS || 
        parser->current_token->type == TOKEN_MINUS_MINUS) {
        node = parse_unary_operator(parser, node, true);
    }
    return node;
}

/**
 * Parses a factor expression from the input tokens.
 *
 * This function handles parsing of factor expressions, which are composed
 * of unary expressions joined by multiplication (*) or division (/)
 * operators. It supports left-associative chaining of these operators,
 * creating a binary expression tree as a result.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed factor expression.
 */
static AST *parse_factor(Parser *parser) {
    AST *left = parse_unary(parser);

    while (parser->current_token->type == TOKEN_STAR || 
           parser->current_token->type == TOKEN_SLASH) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_unary(parser);
        left = CREATE_AST_STR(
            AST_BINARY_EXPR, left, right, 
            get_token_str(operator),
            parser->current_token->srcloc);
    }
    return left;
}

/**
 * Parses a bitwise expression from the input tokens.
 *
 * This function handles parsing of bitwise expressions, which are composed
 * of term expressions joined by bitwise AND (&) or bitwise OR (|)
 * operators. It supports left-associative chaining of these operators,
 * creating a binary expression tree as a result.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed bitwise expression.
 */
static AST *parse_bitwise(Parser *parser) {
    AST *left = parse_term(parser);

    while (parser->current_token->type == TOKEN_AMP || 
           parser->current_token->type == TOKEN_PIPE) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_factor(parser);
        left = CREATE_AST_STR(
            AST_BINARY_EXPR, left, right, 
            get_token_str(operator),
            parser->current_token->srcloc);
    }
    return left;
}

/**
 * Parses a term expression from the input tokens.
 *
 * This function handles parsing of term expressions, which are composed
 * of factor expressions joined by addition (+) or subtraction (-)
 * operators. It supports left-associative chaining of these operators,
 * creating a binary expression tree as a result.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed term expression.
 */
static AST *parse_term(Parser *parser) {
    AST *left = parse_factor(parser);

    while (parser->current_token->type == TOKEN_PLUS || 
           parser->current_token->type == TOKEN_MINUS) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_factor(parser);
        left = CREATE_AST_STR(
            AST_BINARY_EXPR, left, right,
            get_token_str(operator),
            parser->current_token->srcloc);
    }
    return left;
}

/**
 * Parses a comparison expression from the input tokens.
 *
 * This function handles parsing of comparison expressions, which are
 * composed of bitwise expressions joined by comparison operators.
 * It supports left-associative chaining of these operators, creating
 * a binary expression tree as a result.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed comparison
 *         expression.
 */
static AST *parse_comparison(Parser *parser) {
    AST *left = parse_bitwise(parser);
    TokenType token_type = parser->current_token->type;

    while (token_type == TOKEN_LESS || 
           token_type == TOKEN_GREATER || 
           token_type == TOKEN_LESS_EQUAL || 
           token_type == TOKEN_GREATER_EQUAL) {
        TokenType operator = parser->current_token->type;

        consume(parser, operator);
        AST *right = parse_bitwise(parser);        
        left = CREATE_AST_STR(
            AST_BINARY_EXPR, left, right, 
            get_token_str(token_type),
            parser->current_token->srcloc);
        token_type = parser->current_token->type;
    }
    return left;
}

/**
 * Parses an object literal from the input tokens.
 *
 * This function parses an object literal into a linked list of
 * AST nodes, with each node of type AST_OBJECT. If a syntax error
 * is encountered, an error node is returned, and the current token
 * is consumed to recover.
 *
 * @param parser A pointer to the Parser containing the tokens to
 *               be parsed.
 * @return A pointer to an AST node representing the parsed object
 *         literal, or an error node if a syntax error is encountered.
 */
static AST *parse_object_literal(Parser *parser, ApexString *name) {
    AST *node = CREATE_AST_STR(
        AST_OBJECT, NULL, NULL, name,
        parser->current_token->srcloc);

    consume(parser, TOKEN_LBRACE);

    while (parser->current_token->type != TOKEN_RBRACE) {
        AST *key = NULL;
        AST *value = NULL;

        if (parser->current_token->type == TOKEN_IDENT) {
            key = CREATE_AST_STR(
                AST_STR, NULL, NULL, 
                parser->current_token->str,
                parser->current_token->srcloc);
            consume(parser, TOKEN_IDENT);

            consume(parser, TOKEN_EQUAL);
            value = parse_expression(parser);

            AST *key_value_pair = CREATE_AST_ZERO(
                AST_KEY_VALUE_PAIR, key, value, 
                parser->current_token->srcloc);
            node->right = append_ast(node->right, key_value_pair);
        }

        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        } else if (parser->current_token->type != TOKEN_RBRACE) {
            apexErr_syntax(parser->lexer, "Expected ',' or '}' in object literal");
            consume(parser, parser->current_token->type);
        }
    }

    consume(parser, TOKEN_RBRACE);
    return node;
}

/**
 * Parses an assignment expression from the input tokens.
 *
 * This function handles parsing of assignment expressions, which consist
 * of an identifier on the left side and an expression on the right side.
 * The assignment operator can be either '=' or one of the compound assignment
 * operators ('+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '&=', '^=', '|=').
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed assignment expression.
 */
static AST *parse_assignment(Parser *parser) {
    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer, "invalid assignment target");
    }

    AST *left = parse_primary(parser);

    if (parser->current_token->type == TOKEN_EQUAL || 
        parser->current_token->type == TOKEN_PLUS_EQUAL || 
        parser->current_token->type == TOKEN_MINUS_EQUAL || 
        parser->current_token->type == TOKEN_STAR_EQUAL || 
        parser->current_token->type == TOKEN_SLASH_EQUAL) {        
        TokenType operator = parser->current_token->type;
        ApexString *operator_str = get_token_str(operator);
        
        consume(parser, operator);  
        // Check if the assignment is an array or object literal
        if (parser->current_token->type == TOKEN_LBRACKET) {
            AST *array_literal = parse_primary(parser);
            return CREATE_AST_STR(
                AST_ASSIGNMENT, left, array_literal, 
                operator_str,
                parser->current_token->srcloc);
        } else if (parser->current_token->type == TOKEN_LBRACE) {
            AST *object_literal = parse_object_literal(parser, left->value.strval);
            return CREATE_AST_STR(
                AST_ASSIGNMENT, left, object_literal, 
                operator_str,
                parser->current_token->srcloc);
        } else {
            AST *value = parse_expression(parser);
            return CREATE_AST_STR(
                AST_ASSIGNMENT, left, value,
                operator_str,
                parser->current_token->srcloc);
        }
    // Check if the identifier is an array access
    } else if (parser->current_token->type == TOKEN_LBRACKET) {
        consume(parser, TOKEN_LBRACKET);
        AST *index = parse_expression(parser);
        consume(parser, TOKEN_RBRACKET);

        if (parser->current_token->type != TOKEN_EQUAL) {
            apexErr_syntax(parser->lexer, "expected '=' after array index in assignment");
        }
        consume(parser, TOKEN_EQUAL);

        AST *value = parse_expression(parser);
        AST *array_access = CREATE_AST_ZERO(
            AST_ARRAY_ACCESS, left, index,
            parser->current_token->srcloc);

        return CREATE_AST_STR(
            AST_ASSIGNMENT, array_access, value, 
            apexStr_new("=", 1),
            parser->current_token->srcloc);
    }
    apexErr_syntax(parser->lexer, 
        "unexpected token '%s' in assignment",
        parser->current_token->str->value);
}

/**
 * Parses a block of statements from the input tokens.
 *
 * This function assumes that the current token is the start of a block,
 * indicated by a left brace '{'. It parses all statements within the
 * block until a right brace '}' or the end of file is encountered.
 * Each statement is linked together in a right-linked list of AST nodes.
 *
 * If there are no statements in the block, the function returns an
 * AST_BLOCK node with a NULL first statement. Syntax errors encountered
 * during parsing are handled by skipping tokens until the block ends.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the block of statements,
 *         with each statement linked in a right-linked list.
 */
static AST *parse_block(Parser *parser) {
    AST *first_stmt = NULL;
    AST *last_stmt = NULL;
    SrcLoc srcloc = parser->current_token->srcloc;

    consume(parser, TOKEN_LBRACE);

    while (parser->current_token->type != TOKEN_RBRACE && 
           parser->current_token->type != TOKEN_EOF) {
        AST *stmt = parse_statement(parser);

        if (!stmt) {
            continue;
        }

        if (!first_stmt) {
            first_stmt = stmt;
        } else {
            last_stmt->right = stmt;
        }
        last_stmt = stmt;
    }

    consume(parser, TOKEN_RBRACE);
    return CREATE_AST_ZERO(AST_BLOCK, first_stmt, NULL, srcloc);
}

/**
 * Parses an include statement from the input tokens.
 *
 * This function assumes that the current token is the 'include' keyword, and
 * will parse the file path string following the keyword. If the file path is
 * not a string, a syntax error is reported and an error node is returned.
 * The function returns an AST node representing the include statement, with
 * the file path stored as a string value.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed include statement.
 */
static AST *parse_include(Parser *parser) {
    consume(parser, TOKEN_INCLUDE);

    if (parser->current_token->type != TOKEN_STR) {
        apexErr_syntax(parser->lexer, "expected file path after 'include'");
    }

    ApexString *filepath = parser->current_token->str;
    consume(parser, TOKEN_STR);

    return CREATE_AST_STR(
        AST_INCLUDE, NULL, NULL, filepath,
        parser->current_token->srcloc);
}

/**
 * Parses an if statement from the input tokens.
 *
 * This function assumes that the current token is the start of an if statement,
 * and will parse the condition, then branch, and optionally any elif and else
 * branches. The condition must be enclosed in parentheses, and the branches
 * can be blocks enclosed in braces or single statements.
 *
 * The function handles nested elif statements by chaining them together in the
 * AST. If an else branch is present, it is included as the right child of the
 * last elif or then branch.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the if statement, or an error
 *         node if a syntax error is encountered.
 */
static AST *parse_if_statement(Parser *parser) {
    AST *then_branch = NULL;
    AST *else_branch = NULL;
    AST *current_else = NULL;
    SrcLoc srcloc = parser->current_token->srcloc;

    consume(parser, TOKEN_IF);
    consume(parser, TOKEN_LPAREN);
    AST *condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN);

    if (parser->current_token->type == TOKEN_LBRACE) {
        then_branch = parse_block(parser);
    } else {
        then_branch = parse_statement(parser);
    }
    
    while (parser->current_token->type == TOKEN_ELIF) {
        AST *elif_then_branch = NULL;
        AST *elif_node;

        consume(parser, TOKEN_ELIF);
        consume(parser, TOKEN_LPAREN);
        AST *elif_condition = parse_expression(parser);
        consume(parser, TOKEN_RPAREN);

         if (parser->current_token->type == TOKEN_LBRACE) {
            elif_then_branch = parse_block(parser);
        } else {
            elif_then_branch = parse_statement(parser);
        }

        elif_node = CREATE_AST_ZERO(
            AST_IF, 
            elif_condition, 
            elif_then_branch, 
            parser->current_token->srcloc);

        if (current_else) {
            current_else->value.ast_node = elif_node;
        } else if (!else_branch) {
            else_branch = elif_node;
        } else {
            apexErr_syntax(parser->lexer, "unexpected 'elif' without a preceding if/else");
        }

        current_else = elif_node;
    }

    if (parser->current_token->type == TOKEN_ELSE) {
        AST *final_else_branch = NULL;
        consume(parser, TOKEN_ELSE);
        if (parser->current_token->type == TOKEN_LBRACE) {
            final_else_branch = parse_block(parser);
        } else {
            final_else_branch = parse_statement(parser);
        }
        if (current_else) {
            current_else->value.ast_node = final_else_branch;
        } else if (!else_branch) {
            else_branch = final_else_branch;
        }
    }

    return CREATE_AST_AST(
        AST_IF, condition, then_branch,
        else_branch, srcloc);
}

/**
 * Parses a while statement from the input tokens.
 *
 * This function assumes that the current token is the start of a while
 * statement, and will parse the condition, and the body of the loop.
 * The condition is parsed as an expression, and the body is parsed as
 * either a block or a single statement. If a syntax error is encountered
 * during parsing, the function will recover by consuming tokens until
 * the end of the loop is reached.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed while statement.
 */
static AST *parse_while_statement(Parser *parser) {
    consume(parser, TOKEN_WHILE);
    consume(parser, TOKEN_LPAREN);
    AST *condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN);
    
    AST *body;
    if (parser->current_token->type == TOKEN_LBRACE) {
        body = parse_block(parser);
    } else {
        body = parse_statement(parser);
    }
    
    return CREATE_AST_ZERO(
        AST_WHILE, condition, body,
        parser->current_token->srcloc);
}

/**
 * Parses a for statement from the input tokens.
 *
 * This function assumes that the current token is the start of a for
 * statement, and will parse the initialization, condition, increment,
 * and body of the loop. The initialization, condition, and increment
 * are parsed as expressions, and the body is parsed as either a block
 * or a single statement. If a syntax error is encountered during
 * parsing, the function will recover by consuming tokens until the
 * end of the loop is reached.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed for statement.
 */
static AST *parse_for_statement(Parser *parser) {
    consume(parser, TOKEN_FOR);
    consume(parser, TOKEN_LPAREN);

    AST *initialization = NULL;
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        initialization = parse_statement(parser);
    } else {    
        consume(parser, TOKEN_SEMICOLON);
    }
    
    AST *condition = NULL;
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        condition = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON);
    
    AST *increment = NULL;
    if (parser->current_token->type != TOKEN_RPAREN) {
        increment = parse_expression(parser);
    }
    consume(parser, TOKEN_RPAREN);

    AST *body = NULL;
    if (parser->current_token->type == TOKEN_LBRACE) {
        body = parse_block(parser);
    } else {
        body = parse_statement(parser);
    }
    
    return CREATE_AST_AST(
        AST_FOR, initialization, condition, 
        CREATE_AST_ZERO(
            AST_BLOCK, increment, body,
            parser->current_token->srcloc),
        parser->current_token->srcloc);
}

/**
 * Parses a return statement from the input tokens.
 *
 * This function assumes that the current token is the start of a return
 * statement and will parse the expression following the 'return' keyword
 * if it exists. The expression is optional and the function handles both
 * cases where an expression is present or absent. The statement is expected
 * to end with a semicolon. If a syntax error is encountered, the function
 * will still consume tokens to reach the semicolon.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the return statement with
 *         an optional expression, or an error node if a syntax error is
 *         encountered.
 */
static AST *parse_return_statement(Parser *parser) {
    AST *expression = NULL;

    consume(parser, TOKEN_RETURN);
    
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        expression = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON);
    return CREATE_AST_ZERO(
        AST_RETURN, expression, NULL,
        parser->current_token->srcloc);
}

/**
 * Parses a function declaration from the input tokens.
 *
 * This function assumes that the current token is the start of a function
 * declaration, indicated by the 'fn' keyword. It parses the function name,
 * parameter list, and body enclosed in braces. The function name must be
 * an identifier, and the parameter list is enclosed in parentheses with
 * parameters separated by commas. If the syntax is incorrect at any point,
 * a syntax error is reported.
 *
 * The function body is parsed as a block of statements. The resulting AST
 * node represents the function declaration with its name, parameters, and
 * body.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed function declaration,
 *         or an error node if a syntax error is encountered.
 */
static AST *parse_function_declaration(Parser *parser) {
    SrcLoc srcloc = parser->current_token->srcloc;

    consume(parser, TOKEN_FN);
    
    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer, "expected function name after 'fn'");
    }

    AST *name = CREATE_AST_STR(
        AST_VAR, NULL, NULL,
        parser->current_token->str,
        parser->current_token->srcloc);

    consume(parser, TOKEN_IDENT);    

    // Check if this is a member function or 'new'
    if (parser->current_token->type == TOKEN_DOT) {
        consume(parser, TOKEN_DOT);
        if (parser->current_token->type != TOKEN_IDENT) {
            apexErr_syntax(parser->lexer, "expected member function name after '.'");
        }
        if (parser->current_token->str == apexStr_new("new", 4)) {
            consume(parser, TOKEN_IDENT);
            name = CREATE_AST_STR(
                AST_CTOR, name, NULL,
                parser->current_token->str,
                srcloc);
        } else {
            AST *member_name = CREATE_AST_STR(
                AST_VAR, NULL, NULL,
                parser->current_token->str,
                parser->current_token->srcloc);
            name = CREATE_AST_ZERO(
                AST_MEMBER_FN, name, member_name,
                srcloc);
            consume(parser, TOKEN_IDENT);
        }
    }

    consume(parser, TOKEN_LPAREN);
    
    AST *parameters = NULL;
    while (parser->current_token->type != TOKEN_RPAREN) {
        if (parser->current_token->type != TOKEN_IDENT) {
            apexErr_syntax(parser->lexer, "expected parameter name");
        }

        AST *param = CREATE_AST_STR(
            AST_VAR, NULL, NULL,
            parser->current_token->str,
            parser->current_token->srcloc);

        parameters = CREATE_AST_ZERO(
            AST_PARAMETER_LIST, param, parameters,
            parser->current_token->srcloc);

        consume(parser, TOKEN_IDENT);
        
        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        }
    }
    consume(parser, TOKEN_RPAREN);
    
    AST *body = parse_block(parser);
    return CREATE_AST_AST(
        AST_FN_DECL, name, body, 
        parameters, srcloc);
}

/**
 * Parses a single statement from the input tokens.
 *
 * This function handles parsing of single statements, including
 * if/else statements, while loops, for loops, function declarations,
 * assignments, and function calls. It also handles parsing of
 * break/continue statements.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the parsed statement, or NULL
 *         if a syntax error is encountered.
 */
static AST *parse_statement(Parser *parser) {
    AST *stmt = NULL;
    Token next_token;
    SrcLoc srcloc = parser->current_token->srcloc;

    switch (parser->current_token->type) {
    case TOKEN_IF:
        stmt = parse_if_statement(parser);
        break;    

    case TOKEN_WHILE:
        stmt = parse_while_statement(parser);
        break;    

    case TOKEN_FOR:
        stmt = parse_for_statement(parser);
        break;

    case TOKEN_FN:
        stmt = parse_function_declaration(parser);
        break;

    case TOKEN_RETURN:
        stmt = parse_return_statement(parser);
        break;

    case TOKEN_IDENT:
        next_token = peek_token(parser, 1);

        if (next_token.type == TOKEN_EQUAL || 
            next_token.type == TOKEN_PLUS_EQUAL || 
            next_token.type == TOKEN_MINUS_EQUAL || 
            next_token.type == TOKEN_STAR_EQUAL || 
            next_token.type == TOKEN_SLASH_EQUAL) {            
            stmt = parse_assignment(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        } else if (next_token.type == TOKEN_DOT) {
            for (int i = 2; next_token.type != TOKEN_EOF; i++) {
                next_token = peek_token(parser, i);                

                if (next_token.type == TOKEN_EQUAL || 
                    next_token.type == TOKEN_PLUS_EQUAL || 
                    next_token.type == TOKEN_MINUS_EQUAL || 
                    next_token.type == TOKEN_STAR_EQUAL || 
                    next_token.type == TOKEN_SLASH_EQUAL) {
                    stmt = parse_assignment(parser);
                    consume(parser, TOKEN_SEMICOLON);
                    break;
                }
                if (next_token.type == TOKEN_SEMICOLON) {
                    stmt = parse_expression(parser);
                    consume(parser, TOKEN_SEMICOLON);
                    break;
                }
            }
            break;
        } else if (next_token.type == TOKEN_LBRACKET) {
            int i;
            for (i = 0; next_token.type != TOKEN_RBRACKET; i++) {
                next_token = peek_token(parser, i);
            }
            next_token = peek_token(parser, i);
            if (next_token.type == TOKEN_PLUS_PLUS || 
                next_token.type == TOKEN_MINUS_MINUS) {
                stmt = parse_expression(parser);
            } else {
                stmt = parse_assignment(parser);
            }
            consume(parser, TOKEN_SEMICOLON);
            break;
        } else if (next_token.type == TOKEN_LPAREN) {
            stmt = parse_function_call(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        } else {
            stmt = parse_expression(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        }

    case TOKEN_CONTINUE:
        consume(parser, TOKEN_CONTINUE);
        stmt = CREATE_AST_ZERO(
            AST_CONTINUE, NULL, NULL,
            parser->current_token->srcloc);
        consume(parser, TOKEN_SEMICOLON);
        break;

    case TOKEN_BREAK:
        consume(parser, TOKEN_BREAK);
        stmt = CREATE_AST_ZERO(
            AST_BREAK, NULL, NULL, 
            parser->current_token->srcloc);
        consume(parser, TOKEN_SEMICOLON);
        break;

    case TOKEN_INCLUDE:
        stmt = parse_include(parser);
        consume(parser, TOKEN_SEMICOLON);
        break;
        
    default:
        stmt = parse_expression(parser);
        consume(parser, TOKEN_SEMICOLON);
        break;
    }

    if (stmt && stmt->type != AST_STATEMENT) { 
        stmt = CREATE_AST_ZERO(
            AST_STATEMENT, stmt, NULL,
            srcloc);
    }

    return stmt;
}

/**
 * Parses the entire program from the input tokens.
 *
 * This function assumes that the current token is the start of a program, and
 * will parse all statements until the end of file is reached.
 *
 * The function handles nested statements by linking them together in a
 * linked list of AST nodes. The return value is the head of this list.
 *
 * @param parser A pointer to the Parser containing the tokens to be parsed.
 * @return A pointer to an AST node representing the program, or an error node
 *         if a syntax error is encountered.
 */
AST *parse_program(Parser *parser) {
    AST *program = NULL;
    AST *last_stmt = NULL;

    while (parser->current_token->type != TOKEN_EOF) {
        AST *stmt = parse_statement(parser);

        if (!stmt) {
            continue;
        }

        if (!program) {
            program = stmt;
        } 

        if (last_stmt) {
            last_stmt->right = stmt;
        }       
        last_stmt = stmt;
        
    }
    return program;
}

void free_parser(Parser *parser) {
    free_token(parser->current_token);
}