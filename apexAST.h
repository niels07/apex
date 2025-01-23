#ifndef APEX_AST_H
#define APEX_AST_H

#include <stdbool.h>
#include "apexLex.h"
#include "apexStr.h"

typedef enum {
    AST_ERROR,
    AST_INT,
    AST_DBL,
    AST_STR,    
    AST_BOOL,
    AST_NULL,
    AST_BIN_ADD,
    AST_BIN_SUB,
    AST_BIN_MUL,
    AST_BIN_DIV,
    AST_BIN_MOD,
    AST_BIN_GT,
    AST_BIN_LT,
    AST_BIN_LE,
    AST_BIN_GE,
    AST_BIN_BITWISE_AND,
    AST_BIN_BITWISE_OR,
    AST_BIN_EQ,
    AST_BIN_NE,
    AST_UNARY_ADD,
    AST_UNARY_SUB,
    AST_UNARY_NOT,
    AST_UNARY_INC,
    AST_UNARY_DEC,
    AST_ASSIGN_ADD,
    AST_ASSIGN_SUB,
    AST_ASSIGN_MUL,
    AST_ASSIGN_DIV,
    AST_ASSIGN_MOD,
    AST_LOGICAL_EXPR,
    AST_VAR,
    AST_VARIADIC,
    AST_ASSIGNMENT,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FOREACH,
    AST_FOREACH_IT,
    AST_FN_DECL,
    AST_FN_CALL,
    AST_LIB_CALL,
    AST_PARAMETER_LIST,
    AST_ARGUMENT_LIST,
    AST_ARRAY,
    AST_ARRAY_ACCESS,
    AST_KEY_VALUE_PAIR,
    AST_ELEMENT,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_STATEMENT,
    AST_INCLUDE,
    AST_MEMBER_ACCESS,
    AST_MEMBER_FN,
    AST_CTOR,
    AST_NEW,
    AST_CLOSURE,
    AST_OBJECT,
    AST_OBJ_FIELD,
    AST_SWITCH,
    AST_CASE,
    AST_DEFAULT
} ASTNodeType;

typedef union {
    ApexString *strval; 
    struct AST *ast_node;
} ASTValue;

typedef struct AST {
    ASTNodeType type;
    struct AST *left;
    struct AST *right;
    struct AST *next;
    ASTValue value;
    SrcLoc srcloc;
    bool val_is_ast;
} AST;

#define CREATE_AST_STR(type, left, right, value, srcloc) create_ast_node(type, left, right, ast_value_str(value), false, srcloc)
#define CREATE_AST_AST(type, left, right, value, srcloc) create_ast_node(type, left, right, ast_value_ast(value), true, srcloc)
#define CREATE_AST_ZERO(type, left, right, srcloc) create_ast_node(type, left, right, ast_value_zero(), false, srcloc)

extern void print_ast(AST *node, int indent);
extern AST *create_ast_node(ASTNodeType type, AST *left, AST *right, ASTValue value, bool val_is_ast, SrcLoc srcloc);
extern AST *create_error_ast(void);
extern ASTValue ast_value_zero(void);
extern ASTValue ast_value_str(ApexString *str);
extern ASTValue ast_value_ast(AST *ast);
extern void free_ast(AST *node);
extern void print_ast(AST *node, int indent);

#endif
