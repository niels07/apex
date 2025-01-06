#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "srcloc.h"
#include "string.h"

typedef enum {
    AST_ERROR,
    AST_INT,
    AST_DBL,
    AST_STR,    
    AST_BOOL,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_LOGICAL_EXPR,
    AST_VAR,
    AST_ASSIGNMENT,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FN_DECL,
    AST_FN_CALL,
    AST_PARAMETER_LIST,
    AST_ARGUMENT_LIST,
    AST_ARRAY,
    AST_ARRAY_ACCESS,
    AST_KEY_VALUE_PAIR,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_STATEMENT,
    AST_INCLUDE,
    AST_MEMBER_ACCESS,
    AST_MEMBER_FN,
    AST_CTOR,
    AST_NEW,
    AST_OBJECT
} ASTNodeType;

typedef union {
    ApexString *strval; 
    struct AST *ast_node;
} ASTValue;

typedef struct AST {
    ASTNodeType type;
    struct AST *left;
    struct AST *right;
    ASTValue value;
    SrcLoc srcloc;
    bool val_is_ast;
} AST;

#define CREATE_AST_STR(type, left, right, value, srcloc) create_ast_node(type, left, right, ast_value_str(value), false, srcloc)
#define CREATE_AST_AST(type, left, right, value, srcloc) create_ast_node(type, left, right, ast_value_ast(value), true, srcloc)
#define CREATE_AST_ZERO(type, left, right, srcloc) create_ast_node(type, left, right, ast_value_zero(), false, srcloc)

extern void print_ast(AST *node, int indent);
extern AST *create_ast_node(ASTNodeType type, AST *left, AST *right, ASTValue value, bool val_is_ast, SrcLoc srcloc);
extern ASTValue ast_value_zero(void);
extern ASTValue ast_value_str(ApexString *str);
extern ASTValue ast_value_ast(AST *ast);
extern void free_ast(AST *node);
extern void print_ast(AST *node, int indent);

#endif
