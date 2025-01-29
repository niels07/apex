#ifndef APEX_AST_H
#define APEX_AST_H

#include <stdbool.h>
#include "apexLex.h"
#include "apexStr.h"

/**
 * Enumerates the possible node types for the abstract syntax tree.
 */
typedef enum {
    AST_ERROR,               /** AST error */
    AST_INT,                 /** AST integer */
    AST_DBL,                 /** AST double */
    AST_STR,                 /** AST string */
    AST_BOOL,                /** AST boolean */
    AST_NULL,                /** AST null */
    AST_BIN_ADD,             /** AST binary addition */
    AST_BIN_SUB,             /** AST binary subtraction */
    AST_BIN_MUL,             /** AST binary multiplication */
    AST_BIN_DIV,             /** AST binary division */
    AST_BIN_MOD,             /** AST binary modulus */
    AST_BIN_GT,              /** AST binary greater than */
    AST_BIN_LT,              /** AST binary less than */
    AST_BIN_LE,              /** AST binary less or equal */
    AST_BIN_GE,              /** AST binary greater or equal */
    AST_BIN_BITWISE_AND,     /** AST binary bitwise and */
    AST_BIN_BITWISE_OR,      /** AST binary bitwise or */
    AST_BIN_EQ,              /** AST binary equality */
    AST_BIN_NE,              /** AST binary not equal */
    AST_UNARY_ADD,           /** AST unary addition */
    AST_UNARY_SUB,           /** AST unary subtraction */
    AST_UNARY_NOT,           /** AST unary not */
    AST_UNARY_INC,           /** AST unary increment */
    AST_UNARY_DEC,           /** AST unary decrement */
    AST_ASSIGN_ADD,          /** AST assignment addition */
    AST_ASSIGN_SUB,          /** AST assignment subtraction */
    AST_ASSIGN_MUL,          /** AST assignment multiplication */
    AST_ASSIGN_DIV,          /** AST assignment division */
    AST_ASSIGN_MOD,          /** AST assignment modulus */
    AST_LOGICAL_EXPR,        /** AST logical expression */
    AST_VAR,                 /** AST variable */
    AST_VARIADIC,            /** AST variadic */
    AST_ASSIGNMENT,          /** AST assignment */
    AST_BLOCK,               /** AST block */
    AST_TERNARY,             /** AST ternary */
    AST_IF,                  /** AST if statement */
    AST_WHILE,               /** AST while loop */
    AST_FOR,                 /** AST for loop */
    AST_FOREACH,             /** AST foreach loop */
    AST_FOREACH_IT,          /** AST foreach iterator */
    AST_FN_DECL,             /** AST function declaration */
    AST_FN_CALL,             /** AST function call */
    AST_LIB_CALL,            /** AST library call */
    AST_LIB_MEMBER,          /** AST Library member */
    AST_PARAMETER_LIST,      /** AST parameter list */
    AST_ARGUMENT_LIST,       /** AST argument list */
    AST_ARRAY,               /** AST array */
    AST_ARRAY_ACCESS,        /** AST array access */
    AST_KEY_VALUE_PAIR,      /** AST key-value pair */
    AST_ELEMENT,             /** AST element */
    AST_RETURN,              /** AST return */
    AST_BREAK,               /** AST break */
    AST_CONTINUE,            /** AST continue */
    AST_STATEMENT,           /** AST statement */
    AST_INCLUDE,             /** AST include */
    AST_MEMBER_ACCESS,       /** AST member access */
    AST_MEMBER_FN,           /** AST member function */
    AST_CTOR,                /** AST constructor */
    AST_NEW,                 /** AST new */
    AST_CLOSURE,             /** AST closure */
    AST_OBJECT,              /** AST object */
    AST_OBJ_FIELD,           /** AST object field */
    AST_SWITCH,              /** AST switch */
    AST_CASE,                /** AST case */
    AST_DEFAULT              /** AST default */
} ASTNodeType;

/**
 * Union that can hold either a string or an abstract syntax tree node.
 *
 * Used to represent values associated with nodes in the abstract syntax
 * tree.
 */
typedef union {
    /**
     * String value.
     */
    ApexString *strval; 
    /**
     * Abstract syntax tree node value.
     */
    struct AST *ast_node;
} ASTValue;

/**
 * Abstract syntax tree node structure.
 *
 * Each node in the abstract syntax tree has a type, left and right children, a
 * linked list of siblings, a value associated with the node, and a source
 * location.
 */
typedef struct AST {
    /**
     * Type of the node.
     */
    ASTNodeType type;
    /**
     * Left child of the node.
     */
    struct AST *left;
    /**
     * Right child of the node.
     */
    struct AST *right;
    /**
     * Next sibling of the node.
     */
    struct AST *next;
    /**
     * Value associated with the node.
     */
    ASTValue value;
    /**
     * Source location of the node.
     */
    SrcLoc srcloc;
    /**
     * Flag that indicates whether the value is an abstract syntax tree node
     * or a string.
     */
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
