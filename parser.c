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
 * Peeks at the next token in the input without consuming it.
 *
 * This function creates a backup of the current lexer state and retrieves
 * the next token using the backup. The current lexer's position remains
 * unchanged, allowing the parser to look ahead without affecting the
 * parsing process.
 *
 * @param parser A pointer to the Parser containing the lexer and tokens.
 * @return The next Token in the input sequence, without advancing the lexer.
 */
static Token peek_next_token(Parser *parser) {
    Lexer backup_lexer = *parser->lexer;
    Token *next_token = get_next_token(&backup_lexer);
    Token next = *next_token;
    free_token(next_token);
    return next;
}

/**
 * Consumes the current token if it matches the given type, and consumes
 * the next token from the lexer. If the current token does not match the
 * given type, an error is reported to the user.
 *
 * @param parser A pointer to a Parser containing the tokens to be parsed.
 * @param type The token type to be consumed.
 */
static void consume(Parser *parser, TokenType type) {
    if (parser->current_token->type == type) {
        free_token(parser->current_token);
        parser->current_token = get_next_token(parser->lexer);
    } else {
        apexErr_syntax(
            parser->lexer->line, 
            "expected '%s' but found '%s'", 
            get_token_str(type), 
            parser->current_token->value);
    }
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
        AST *right;

        consume(parser, operator);
        right = parse_comparison(parser);
        left = create_ast_node(
            AST_BINARY_EXPR, left, right,
            ast_value_str(get_token_str(operator)), 
            parser->current_token->lineno);
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
        AST *right;

        consume(parser, operator);
        right = parse_equality(parser);
        left = create_ast_node(
            AST_LOGICAL_EXPR, left, right, 
            ast_value_str(get_token_str(operator)),
            parser->current_token->lineno);
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
    AST *function_name = NULL;
    AST *arguments = NULL;

    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer->line, "Expected function name before '('.");
        return create_error_ast();
    }

    function_name = create_ast_node(
        AST_VAR, NULL, NULL, 
        ast_value_str(parser->current_token->value),
        parser->current_token->lineno);

    consume(parser, TOKEN_IDENT);

    consume(parser, TOKEN_LPAREN);
    while (parser->current_token->type != TOKEN_RPAREN) {
        AST *argument = parse_expression(parser);

        arguments = create_ast_node(
            AST_ARGUMENT_LIST, arguments, argument,
            ast_value_zero(),
            parser->current_token->lineno);
        
        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        } else if (parser->current_token->type != TOKEN_RPAREN) {
            apexErr_syntax(parser->lexer->line, "Expected ',' or ')' in argument list.");
            consume(parser, parser->current_token->type);
        }
    }
    consume(parser, TOKEN_RPAREN);

    return create_ast_node(
        AST_FN_CALL, function_name, arguments,
        ast_value_zero(),
        parser->current_token->lineno);
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
        node = create_ast_node(
            AST_INT, NULL, NULL, 
            ast_value_str(parser->current_token->value), 
            parser->current_token->lineno);
        consume(parser, TOKEN_INT);
        return node;
    
    case TOKEN_FLT:
        node = create_ast_node(
            AST_FLT, NULL, NULL, 
            ast_value_str(parser->current_token->value), 
            parser->current_token->lineno);
        consume(parser, TOKEN_FLT);
        return node;

    case TOKEN_STR:
        node = create_ast_node(
            AST_STR, NULL, NULL, 
            ast_value_str(parser->current_token->value), 
            parser->current_token->lineno);
        consume(parser, TOKEN_STR);
        return node;

    case TOKEN_TRUE: 
    case TOKEN_FALSE:
        node = create_ast_node(
            AST_BOOL, NULL, NULL, 
            ast_value_str(parser->current_token->value), 
            parser->current_token->lineno);
        consume(parser, parser->current_token->type);
        return node;

    case TOKEN_IDENT:
        if (peek_next_token(parser).type == TOKEN_LPAREN) {
            return parse_function_call(parser);
        }
        node = create_ast_node(
            AST_VAR, NULL, NULL, 
            ast_value_str(parser->current_token->value), 
            parser->current_token->lineno);
        consume(parser, TOKEN_IDENT);
        return node;
    
    case TOKEN_LPAREN:
        consume(parser, TOKEN_LPAREN);
        node = parse_expression(parser);
        consume(parser, TOKEN_RPAREN);
        return node;

    default:
        apexErr_syntax(
            parser->lexer->line, 
            "unexpected '%s' expecting %s, %s, %s, %s, %s, %s or %s", 
            parser->current_token->value,
            get_token_str(TOKEN_INT), 
            get_token_str(TOKEN_FLT), 
            get_token_str(TOKEN_STR),
            get_token_str(TOKEN_TRUE), 
            get_token_str(TOKEN_FALSE), 
            get_token_str(TOKEN_IDENT), 
            get_token_str(TOKEN_LPAREN));

        while (parser->current_token->type != TOKEN_EOF && 
            parser->current_token->type != TOKEN_SEMICOLON && 
            parser->current_token->type != TOKEN_RBRACE) {
            consume(parser, parser->current_token->type);
        }
        return create_error_ast();
    }
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
    if (parser->current_token->type == TOKEN_PLUS_PLUS) {
        consume(parser, TOKEN_PLUS_PLUS);
        return create_ast_node(
            AST_UNARY_EXPR, NULL, 
            parse_primary(parser), 
            ast_value_str(new_string("++", 2)), 
            parser->current_token->lineno);
    }
    if (parser->current_token->type == TOKEN_MINUS_MINUS) {
        consume(parser, TOKEN_MINUS_MINUS);
        return create_ast_node(
            AST_UNARY_EXPR, NULL, 
            parse_primary(parser), 
            ast_value_str(new_string("--", 2)), 
            parser->current_token->lineno);
    }
    return parse_primary(parser);
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
        AST *right;

        consume(parser, operator);
        right = parse_unary(parser);
        left = create_ast_node(
            AST_BINARY_EXPR, left, right, 
            ast_value_str(get_token_str(operator)),
            parser->current_token->lineno);
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
        AST *right;

        consume(parser, operator);
        right = parse_factor(parser);
        left = create_ast_node(
            AST_BINARY_EXPR, left, right, 
            ast_value_str(get_token_str(operator)),
            parser->current_token->lineno);
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
        AST *right;

        consume(parser, operator);
        right = parse_factor(parser);
        left = create_ast_node(
            AST_BINARY_EXPR, left, right,
            ast_value_str(get_token_str(operator)),
            parser->current_token->lineno);
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
        AST *right;

        consume(parser, operator);
        right = parse_bitwise(parser);        
        left = create_ast_node(
            AST_BINARY_EXPR, left, right, 
            ast_value_str(get_token_str(token_type)),
            parser->current_token->lineno);
        token_type = parser->current_token->type;
    }
    return left;
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
    AST *left;

    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer->line, "Invalid assignment target");
        return create_error_ast();
    }

    left = create_ast_node(
        AST_VAR, NULL, NULL, 
        ast_value_str(parser->current_token->value),
        parser->current_token->lineno);

    consume(parser, TOKEN_IDENT);

    if (parser->current_token->type == TOKEN_EQUAL || 
        parser->current_token->type == TOKEN_PLUS_EQUAL || 
        parser->current_token->type == TOKEN_MINUS_EQUAL || 
        parser->current_token->type == TOKEN_STAR_EQUAL || 
        parser->current_token->type == TOKEN_SLASH_EQUAL) {        
        TokenType operator = parser->current_token->type;
        AST *value;
        char *operator_str = get_token_str(operator);
        
        consume(parser, operator);        
        value = parse_expression(parser);

        return create_ast_node(
            AST_ASSIGNMENT, left, value,
            ast_value_str(operator_str),
            parser->current_token->lineno);
    }
    return left;
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
    int lineno = parser->current_token->lineno;

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
    return create_ast_node(AST_BLOCK, first_stmt, NULL, ast_value_zero(), lineno);
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
    AST *condition = NULL;
    AST *then_branch = NULL;
    AST *else_branch = NULL;
    AST *current_else = NULL;
    int lineno = parser->current_token->lineno;

    consume(parser, TOKEN_IF);
    consume(parser, TOKEN_LPAREN);
    condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN);

    if (parser->current_token->type == TOKEN_LBRACE) {
        then_branch = parse_block(parser);
    } else {
        then_branch = parse_statement(parser);
    }
    
    while (parser->current_token->type == TOKEN_ELIF) {
        AST *elif_condition = NULL;
        AST *elif_then_branch = NULL;
        AST *elif_node;

        consume(parser, TOKEN_ELIF);
        consume(parser, TOKEN_LPAREN);
        elif_condition = parse_expression(parser);
        consume(parser, TOKEN_RPAREN);

         if (parser->current_token->type == TOKEN_LBRACE) {
            elif_then_branch = parse_block(parser);
        } else {
            elif_then_branch = parse_statement(parser);
        }

        elif_node = create_ast_node(
            AST_IF, elif_condition, elif_then_branch,
            ast_value_zero(),
            parser->current_token->lineno);

        if (current_else) {
            current_else->value.ast_node = elif_node;
        } else if (!else_branch) {
            else_branch = elif_node;
        } else {
            apexErr_syntax(
                parser->lexer->line, 
                "unexpected 'elif' without a preceding if/else");
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

    return create_ast_node(
        AST_IF, condition, then_branch,
        ast_value_ast(else_branch),
        lineno);
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
    AST *condition = NULL;
    AST *body = NULL;

    consume(parser, TOKEN_WHILE);
    consume(parser, TOKEN_LPAREN);
    condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN);
    
    if (parser->current_token->type == TOKEN_LBRACE) {
        body = parse_block(parser);
    } else {
        body = parse_statement(parser);
    }
    
    return create_ast_node(
        AST_WHILE, condition, body,
        ast_value_zero(),
        parser->current_token->lineno);
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
    AST *initialization = NULL;
    AST *condition = NULL;
    AST *increment = NULL;
    AST *body = NULL;

    consume(parser, TOKEN_FOR);
    consume(parser, TOKEN_LPAREN);
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        initialization = parse_statement(parser);
    }
    consume(parser, TOKEN_SEMICOLON);
    
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        condition = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON);
    
    if (parser->current_token->type != TOKEN_RPAREN) {
        increment = parse_statement(parser);
    }
    consume(parser, TOKEN_RPAREN);
    if (parser->current_token->type == TOKEN_LBRACE) {
        body = parse_block(parser);
    } else {
        body = parse_statement(parser);
    }
    
    return create_ast_node(AST_FOR, initialization, condition, 
        ast_value_ast(
            create_ast_node(
                AST_BLOCK, increment, body,
                ast_value_zero(),
                parser->current_token->lineno)
        ),
        parser->current_token->lineno
    );
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
    return create_ast_node(
        AST_RETURN, expression, NULL,
        ast_value_zero(),
        parser->current_token->lineno);
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
    AST *name = NULL;
    AST *parameters = NULL;
    AST *body = NULL;
    int start_line = parser->current_token->lineno;

    consume(parser, TOKEN_FN);
    
    if (parser->current_token->type != TOKEN_IDENT) {
        apexErr_syntax(parser->lexer->line, "expected function name after 'fn'");
    }

    name = create_ast_node(
        AST_VAR, NULL, NULL,
        ast_value_str(parser->current_token->value),
        parser->current_token->lineno);

    consume(parser, TOKEN_IDENT);    
    consume(parser, TOKEN_LPAREN);
    
    while (parser->current_token->type != TOKEN_RPAREN) {
        AST *param;

        if (parser->current_token->type != TOKEN_IDENT) {
            apexErr_syntax(parser->lexer->line, "expected parameter name");
        }

        param = create_ast_node(
            AST_VAR, NULL, NULL,
            ast_value_str(parser->current_token->value),
            parser->current_token->lineno);

        parameters = create_ast_node(
            AST_PARAMETER_LIST, param, parameters,
            ast_value_zero(),
            parser->current_token->lineno);

        consume(parser, TOKEN_IDENT);
        
        if (parser->current_token->type == TOKEN_COMMA) {
            consume(parser, TOKEN_COMMA);
        }
    }
    consume(parser, TOKEN_RPAREN);
    
    body = parse_block(parser);
    return create_ast_node(
        AST_FN_DECL, name, body, 
        ast_value_ast(parameters),
        start_line);
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
    AST *statement = NULL;
    Token next_token;
    int lineno = parser->current_token->lineno;

    switch (parser->current_token->type) {
    case TOKEN_IF:
        statement = parse_if_statement(parser);
        break;    

    case TOKEN_WHILE:
        statement = parse_while_statement(parser);
        break;    

    case TOKEN_FOR:
        statement = parse_for_statement(parser);
        break;

    case TOKEN_FN:
        statement = parse_function_declaration(parser);
        break;

    case TOKEN_RETURN:
        statement = parse_return_statement(parser);
        break;

    case TOKEN_IDENT:
        next_token = peek_next_token(parser);
        if (next_token.type == TOKEN_EQUAL || 
            next_token.type == TOKEN_PLUS_EQUAL || 
            next_token.type == TOKEN_MINUS_EQUAL || 
            next_token.type == TOKEN_STAR_EQUAL || 
            next_token.type == TOKEN_SLASH_EQUAL) {            
            statement = parse_assignment(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        } else if (next_token.type == TOKEN_LPAREN) {
            statement = parse_function_call(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        } else {
            statement = parse_expression(parser);
            consume(parser, TOKEN_SEMICOLON);
            break;
        }

    case TOKEN_CONTINUE:
        consume(parser, TOKEN_CONTINUE);
        statement = create_ast_node(
            AST_CONTINUE, NULL, NULL, 
            ast_value_zero(), 
            parser->current_token->lineno);
        consume(parser, TOKEN_SEMICOLON);
        break;

    case TOKEN_BREAK:
        consume(parser, TOKEN_BREAK);
        statement = create_ast_node(
            AST_BREAK, NULL, NULL, 
            ast_value_zero(), 
            parser->current_token->lineno);
        consume(parser, TOKEN_SEMICOLON);
        break;
        
    default:
        statement = parse_expression(parser);
        consume(parser, TOKEN_SEMICOLON);
        break;
    }

    if (statement && statement->type != AST_STATEMENT) { 
        statement = create_ast_node(
            AST_STATEMENT, statement, NULL,
            ast_value_zero(),
            lineno);
    }

    return statement;
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