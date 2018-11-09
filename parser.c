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
        const char *expStr = apex_lexer_get_token_str(exp);

        if (i > 0) {
            sprintf(buf, ", '%s'", expStr);
        } else {
            sprintf(buf, "%s'", expStr);
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
    const char *tkstr;

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

    } else if (accept(parser, APEX_TOKEN_INT)) {
        int value = GET_INT(parser);
        apex_ast_int(parser->ast, value);

    } else if (accept(parser, APEX_TOKEN_FLT)) {
        float value = GET_FLT(parser);

    } else if (accept(parser, APEX_TOKEN_STR)) {
        char *value = GET_STR(parser);

    } else if (accept(parser, '(')) {
        expr(parser);
        expect(parser, ')');
    } else {
        unexp(parser, 0);
    }
}

/* un_expr
 * : pri_expr
 * | INC pri_expr
 * | DEC pri_expr
 */
static void un_expr(ApexParser *parser) {
    pri_expr(parser);
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

static void stmt(ApexParser *parser) {
    ApexAstNode *node;

    expr_stmt(parser);
    node = apex_ast_pop(parser->ast);
    apex_ast_opr1(parser->ast, APEX_AST_OPR_PRINT, node);
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
    expect(parser, APEX_TOKEN_IMPORT);
    expect(parser, APEX_TOKEN_STR);
    expect(parser, APEX_TOKEN_END_STMT);
}

/* program
 * : module_spec
 * | program func_decl
 * | program import
 */
static void program(ApexParser *parser) {
    ApexToken tk = NEXT_TOKEN(parser);
    module_spec(parser);

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
