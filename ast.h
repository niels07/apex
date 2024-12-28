#ifndef AST_H
#define AST_H

#include "srcloc.h"

typedef enum {
    AST_ERROR,
    AST_INT,
    AST_FLT,
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
    AST_STATEMENT
} ASTNodeType;

typedef union {
    char *strval; 
    struct AST *ast_node;
} ASTValue;

typedef struct AST {
    ASTNodeType type;
    struct AST *left;
    struct AST *right;
    ASTValue value;
    SrcLoc srcloc;
} AST;

extern void print_ast(AST *node, int indent);
extern AST *create_ast_node(ASTNodeType type, AST *left, AST *right, ASTValue value, SrcLoc srcloc);
extern ASTValue ast_value_zero(void);
extern ASTValue ast_value_str(char *str);
extern ASTValue ast_value_ast(AST *ast);
extern AST *create_error_ast() ;
extern void free_ast(AST *node);
extern void print_ast(AST *node, int indent);

#endif
