#include <stdio.h>
#include <stdarg.h>

#include "ast.h"
#include "vm.h"
#include "error.h"
#include "malloc.h"
#include "parser.h"
#include "value.h"

struct ApexParser {
    ApexLexer *lexer;
    ApexValue *value;
    ApexAst *ast;
};

ApexParser *apex_parser_new(ApexLexer *lexer, ApexAst *ast) {
    ApexParser *parser = APEX_NEW(ApexParser);
    parser->lexer = lexer;
    parser->ast = ast;
    return parser;
}

#define GET_TOKEN(parser) \
    (apex_lexer_get_token((parser)->lexer))

#define NEXT_TOKEN(parser) \
    (apex_lexer_next_token((parser)->lexer))

#define PEEK(p) \
    (apex_lexer_peek((parser)->lexer))

#define GET_INT(parser) \
    APEX_VALUE_INT(parser->value)

#define GET_FLT(parser) \
    APEX_VALUE_FLT(parser->value)

#define GET_STR(parser) \
    APEX_VALUE_STR(parser->value)

static void error(ApexParser *parser, const char *fmt, ...) {
    int lineno = apex_lexer_get_lineno(parser->lexer);
    va_list vl;
    char buf[APEX_ERROR_MAX];

    va_start(vl, fmt);
    vsprintf(buf, fmt, vl);
    va_end(vl);
    apex_error_syntax("%s on line %d", buf, lineno);
}

static void unexp(ApexParser *parser, size_t nExp, ...) {
    char buf[APEX_ERROR_MAX];
    va_list vl;
    size_t i;
    int lineno = apex_lexer_get_lineno(parser->lexer);
    ApexToken tk = GET_TOKEN(parser);
    const char *tkstr = apex_lexer_get_token_str(tk);
    
    if (nExp == 0) {
        sprintf(buf, "unexpected '%s'", tkstr);
    } else {
        sprintf(buf, "unexpected '%s', expecting '", tkstr);
    }

    va_start(vl, nExp);
    for (i = 0; i < nExp; i++) {
        ApexToken exp = va_arg(vl, ApexToken);
        const char *exp_str = apex_lexer_get_token_str(exp);

        if (i > 0) {
            sprintf(buf, "%s, '%s'", buf, exp_str);
        } else {
            sprintf(buf, "'%s'", buf, exp_str);
        }
    }
    va_end(vl);
    apex_error_throw(APEX_ERROR_CODE_SYNTAX, "%s on line %d", buf, lineno);
}

static int accept(ApexParser *parser, ApexToken tk) {
    if (GET_TOKEN(parser) != tk) {
        return 0;
    }
    parser->value = apex_lexer_get_value(parser->lexer);
    NEXT_TOKEN(parser);
    return 1;
}

static int expect(ApexParser *parser, ApexToken exp) {
    ApexToken tk = GET_TOKEN(parser);

    if (tk == exp) {
        parser->value = apex_lexer_get_value(parser->lexer);
        NEXT_TOKEN(parser);
        return 1;
    }
    unexp(parser, 1, exp);
    return 0;
}

static void expr(ApexParser *);


/* pri_expr 
 * : ID
 * | INT
 * | FLT
 * | STR
 * | '(' expr ')'
 */
static void pri_expr(ApexParser *parser) {
    if (accept(parser, APEX_TOKEN_ID)) {
        char *id = GET_STR(parser);
        apex_ast_id(parser->ast, id);

    } else if (accept(parser, APEX_TOKEN_INT)) {
        int value = GET_INT(parser);
        apex_ast_int(parser->ast, value);

    } else if (accept(parser, APEX_TOKEN_FLT)) {
        float value = GET_FLT(parser);

    } else if (accept(parser, APEX_TOKEN_STR)) {
        char *str = GET_STR(parser);
        apex_ast_str(parser->ast, str);

    } else if (accept(parser, '(')) {
        expr(parser);
        expect(parser, ')');
    } else {
        unexp(parser, 0);
    }
}

static void arg_expr_lst(ApexParser *parser) {
    ApexAst *ast = parser->ast;
    ApexAst *list = apex_ast_new();

    parser->ast = list;
    do {
        expr(parser);
    } while (accept(parser, ','));
    parser->ast = ast;
    apex_ast_list(parser->ast, list);
}

/* post_expr
 * : pri_expr
 * | post_expr '[' expr ']'
 * | post_expr '(' ')'
 * | post_expr '(' arg_expr_lst ')'
 * | post_expr '.' ID
 * | post_expr INC
 * | post_expr DEC
 */
static void post_expr(ApexParser *parser) {
    ApexAstNode *left, *right = NULL;

    pri_expr(parser);
    if (accept(parser, '(')) {
        left = apex_ast_pop(parser->ast);
        if (!accept(parser, ')')) {
            arg_expr_lst(parser);   
        }
        expect(parser, ')');
        right = apex_ast_pop(parser->ast);
        apex_ast_opr2(parser->ast, APEX_AST_OPR_CALL, left, right);
    }
}

/* un_expr
 * : post_expr
 * | INC un_expr
 * | DEC un_expr
 */
static void un_expr(ApexParser *parser) {
    post_expr(parser);
}

/* mul_expr
 * : unExpr
 * | mulExpr Mul unExpr
 * | mulExpr Div unExpr 
 * | mulExpr Mod unExpr
 */
static void mul_expr(ApexParser *parser) {
    un_expr(parser);
    for (;;) { 
        ApexAstNode *left, *right;
        ApexAstOpr opr;

        if (accept(parser, '*')) {
            opr = '*';
        } else if (accept(parser, '/')) {
            opr = '/';
        } else if (accept(parser, '%')) {
            opr = '%';
        } else {
            break;
        }
        left = apex_ast_pop(parser->ast);
        un_expr(parser);
        right = apex_ast_pop(parser->ast);
        apex_ast_opr2(parser->ast, opr, left, right);
    }
}

/* add_expr
 * : mul_expr
 * | add_expr ADD mul_expr 
 * | add_expr SUB mul_expr 
 */
static void add_expr(ApexParser *parser) {
    mul_expr(parser);
    for (;;) {
        ApexAstNode *left, *right;
        ApexAstOpr opr;

        if (accept(parser, '+')) {
            opr =  '+';
        } else if (accept(parser, '-')) {
            opr = '-';
        } else {
            break;
        }
        left = apex_ast_pop(parser->ast);
        mul_expr(parser);
        right = apex_ast_pop(parser->ast);
        apex_ast_opr2(parser->ast, opr, left, right);
    }
}

/* expr
 * : asgExpr 
 * | expr ',' asgExpr 
 */
static void expr(ApexParser *p) {
   add_expr(p);
}

/* decln
 * : declnSpec EndStmt
 * | declnSpec initDeclrLst EndStmt
 */
static void decln(ApexParser *p) {

}

/* expr_stmt
 * : END_STMT
 * | expr END_STMT
 */
static void expr_stmt(ApexParser *p) {
    if (accept(p, APEX_TOKEN_END_STMT)) {
        return; 
    }
    expr(p);
    expect(p, APEX_TOKEN_END_STMT);
}

/* stmt
 * : expr_stmt
 */
static void stmt(ApexParser *parser) {
    ApexAstNode *node;

    expr_stmt(parser);
}

/* block 
 * : stmt 
 * | stmt_list stmt 
 */
static void block(ApexParser *parser) {
    do {
        stmt(parser);
    } while (!accept(parser, APEX_TOKEN_END)); 
}

/* module_spec
 * : MODULE STR END_STMT
 */
static void module_spec(ApexParser *parser) {
    expect(parser, APEX_TOKEN_MODULE);
    expect(parser, APEX_TOKEN_STR);
    expect(parser, APEX_TOKEN_END_STMT);
}

/* import
 * : IMPORT STR END_STMT
 */
static void import(ApexParser *parser) {
    char *value;
    ApexAstNode *node;

    expect(parser, APEX_TOKEN_IMPORT);
    expect(parser, APEX_TOKEN_STR);

    value = GET_STR(parser);
    apex_ast_str(parser->ast, value);
    node = apex_ast_pop(parser->ast);
    apex_ast_opr1(parser->ast, APEX_AST_OPR_IMPORT, node);

    expect(parser, APEX_TOKEN_END_STMT);
}

/* program
 * : module_spec
 * | program func_decl
 * | program import
 * | block
 */
static void program(ApexParser *parser) {
    ApexToken tk = NEXT_TOKEN(parser);
/*    module_spec(parser); */

    switch (tk) {
    case APEX_TOKEN_IMPORT:
        import(parser);
        break;

    default:
        block(parser);
        break;
    }
}

void apex_parser_parse(ApexParser *parser) {
    program(parser);
}
