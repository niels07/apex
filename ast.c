#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "mem.h"

static const char *get_ast_node_type_name(ASTNodeType type) {
    switch (type) {
        case AST_ERROR: return "AST_ERROR";
        case AST_INT: return "AST_INT";
        case AST_FLT: return "AST_FLT";
        case AST_STR: return "AST_STR";
        case AST_BOOL: return "AST_BOOL";
        case AST_BINARY_EXPR: return "AST_BINARY_EXPR";
        case AST_UNARY_EXPR: return "AST_UNARY_EXPR";
        case AST_LOGICAL_EXPR: return "AST_LOGICAL_EXPR";
        case AST_VAR: return "AST_VARIABLE";
        case AST_ASSIGNMENT: return "AST_ASSIGNMENT";
        case AST_BLOCK: return "AST_BLOCK";
        case AST_IF: return "AST_IF";
        case AST_WHILE: return "AST_WHILE";
        case AST_FOR: return "AST_FOR";
        case AST_FN_DECL: return "AST_FN_DECL";
        case AST_FN_CALL: return "AST_FN_CALL";
        case AST_PARAMETER_LIST: return "AST_PARAMETER_LIST";
        case AST_ARGUMENT_LIST: return "AST_ARGUMENT_LIST";
        case AST_RETURN: return "AST_RETURN";
        case AST_STATEMENT: return "AST_STATEMENT";
        case AST_BREAK: return "AST_BREAK";
        case AST_CONTINUE: return "AST_CONTINUE";
        case AST_ARRAY: return "AST_ARRAY";
        case AST_ARRAY_ACCESS: return "AST_ARRAY_ACCESS";
        case AST_KEY_VALUE_PAIR: return "AST_KEY_VALUE_PAIR";
        default: return "UNKNOWN_AST_NODE_TYPE";
    }
}

static void print_indent(int indent) {
    int i;
    for (i = 0; i < indent; i++) {
        printf("  ");
    }
}

void print_ast(AST *node, int indent) {
    if (node == NULL) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    printf("Node Type: %s (%d)", get_ast_node_type_name(node->type), node->type);
    
    if (node->value.strval != NULL && node->type != AST_FN_DECL && node->type != AST_IF && node->type != AST_FOR) {
        printf(", Value: \"%s\"", node->value.strval);
    }

    if (node->srcloc.lineno > 0) {
        printf(", Line: %d", node->srcloc.lineno);
    }
    printf("\n");

    if (node->left != NULL) {
        print_indent(indent);
        printf("Left:\n");
        print_ast(node->left, indent + 1);
    }

    if (node->right != NULL) {
        print_indent(indent);
        printf("Right:\n");
        print_ast(node->right, indent + 1);
    }

    if (node->type == AST_FN_DECL || node->type == AST_IF || node->type == AST_FOR) {
        print_indent(indent);
        printf("Value:\n");
        print_ast(node->value.ast_node, indent + 1);
    }
}


/**
 * Creates a new abstract syntax tree node.
 *
 * This function allocates memory for a new abstract syntax tree node and
 * initializes it with the given type, left and right children, value, and
 * source location.
 *
 * @param type The type of the new node.
 * @param left The left child of the new node.
 * @param right The right child of the new node.
 * @param value The value associated with the new node.
 * @param srcloc The source location of the new node.
 *
 * @return A pointer to the newly allocated abstract syntax tree node.
 */
AST *create_ast_node(ASTNodeType type, AST *left, AST *right, ASTValue value, SrcLoc srcloc) {
    AST *node = mem_alloc(sizeof(AST));
    node->type = type;
    node->left = left;
    node->right = right;
    node->value = value;
    node->srcloc = srcloc;
    return node;
}

/**
 * Returns an ASTValue with all fields set to zero.
 *
 * This function can be used to initialize an ASTValue with no value.
 *
 * @return An ASTValue with all fields set to zero.
 */
ASTValue ast_value_zero(void) {
    ASTValue value;
    memset(&value, 0, sizeof(ASTValue));
    return value;
}

/**
 * Creates an ASTValue with the given string value.
 *
 * @param str The string value to associate with the ASTValue.
 *
 * @return An ASTValue with the given string value.
 */
ASTValue ast_value_str(char *str) {
    ASTValue value;
    value.strval = str;
    return value;
}

/**
 * Creates an ASTValue with the given abstract syntax tree node as the value.
 *
 * @param ast The abstract syntax tree node to associate with the ASTValue.
 *
 * @return An ASTValue with the given abstract syntax tree node as the value.
 */
ASTValue ast_value_ast(AST *ast) {
    ASTValue value;
    value.ast_node = ast;
    return value;
}


/**
 * Creates an abstract syntax tree node that represents an error.
 *
 * This function creates an abstract syntax tree node of type AST_ERROR
 * and initializes it with the string value "error". This node has no
 * children and is used to represent a syntax error in an abstract
 * syntax tree.
 *
 * @return A pointer to the newly created abstract syntax tree node that
 *         represents an error.
 */
AST *create_error_ast() {
    AST *node = mem_alloc(sizeof(AST));
    node->type = AST_ERROR;
    node->left = NULL;
    node->right = NULL;
    node->value = ast_value_str("error");
    return node;
}

/**
 * Frees an abstract syntax tree node and all its children.
 *
 * This function recursively calls itself on the left and right children of
 * the given node, and then frees the given node itself.
 *
 * @param node The abstract syntax tree node to be freed. If NULL, this
 *             function does nothing.
 */
void free_ast(AST *node) {
    if (node) {
        free_ast(node->left);
        free_ast(node->right);
        free(node);
    }
}
