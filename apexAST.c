#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apexAST.h"
#include "apexMem.h"

static const char *get_ast_node_type_name(ASTNodeType type) {
    switch (type) {
        case AST_ERROR: return "AST_ERROR";
        case AST_INT: return "AST_INT";
        case AST_DBL: return "AST_DBL";
        case AST_STR: return "AST_STR";
        case AST_BOOL: return "AST_BOOL";
        case AST_NULL: return "AST_NULL";
        case AST_BIN_ADD: return "AST_BIN_ADD";
        case AST_BIN_SUB: return "AST_BIN_SUB";
        case AST_BIN_MUL: return "AST_BIN_MUL";
        case AST_BIN_DIV: return "AST_BIN_DIV";
        case AST_BIN_MOD: return "AST_BIN_MOD";
        case AST_BIN_GT: return "AST_BIN_GT";
        case AST_BIN_LT: return "AST_BIN_LT";
        case AST_BIN_LE: return "AST_BIN_LE";
        case AST_BIN_GE: return "AST_BIN_GE";
        case AST_BIN_BITWISE_AND: return "AST_BIN_BITWISE_AND";
        case AST_BIN_BITWISE_OR: return "AST_BIN_BITWISE_OR";
        case AST_BIN_EQ: return "AST_BIN_EQ";
        case AST_BIN_NE: return "AST_BIN_NE";
        case AST_UNARY_ADD: return "AST_UNARY_ADD";
        case AST_UNARY_SUB: return "AST_UNARY_SUB";
        case AST_UNARY_NOT: return "AST_UNARY_NOT";
        case AST_UNARY_INC: return "AST_UNARY_INC";
        case AST_UNARY_DEC: return "AST_UNARY_DEC";
        case AST_ASSIGN_ADD: return "AST_ASSIGN_ADD";
        case AST_ASSIGN_SUB: return "AST_ASSIGN_SUB";
        case AST_ASSIGN_MUL: return "AST_ASSIGN_MUL";
        case AST_ASSIGN_DIV: return "AST_ASSIGN_DIV";
        case AST_ASSIGN_MOD: return "AST_ASSIGN_MOD";
        case AST_LOGICAL_EXPR: return "AST_LOGICAL_EXPR";
        case AST_VAR: return "AST_VARIABLE";
        case AST_ASSIGNMENT: return "AST_ASSIGNMENT";
        case AST_BLOCK: return "AST_BLOCK";
        case AST_IF: return "AST_IF";
        case AST_WHILE: return "AST_WHILE";
        case AST_FOREACH: return "AST_FOREACH";
        case AST_FOREACH_IT: return "AST_FOREACH_IT";
        case AST_FOR: return "AST_FOR";
        case AST_FN_DECL: return "AST_FN_DECL";
        case AST_FN_CALL: return "AST_FN_CALL";
        case AST_LIB_CALL: return "AST_LIB_CALL";
        case AST_PARAMETER_LIST: return "AST_PARAMETER_LIST";
        case AST_ARGUMENT_LIST: return "AST_ARGUMENT_LIST";
        case AST_RETURN: return "AST_RETURN";
        case AST_STATEMENT: return "AST_STATEMENT";
        case AST_BREAK: return "AST_BREAK";
        case AST_CONTINUE: return "AST_CONTINUE";
        case AST_ARRAY: return "AST_ARRAY";
        case AST_ARRAY_ACCESS: return "AST_ARRAY_ACCESS";
        case AST_KEY_VALUE_PAIR: return "AST_KEY_VALUE_PAIR";
        case AST_INCLUDE: return "AST_INCLUDE";
        case AST_MEMBER_ACCESS: return "AST_MEMBER_ACCESS";
        case AST_MEMBER_FN: return "AST_MEMBER_FN";
        case AST_CTOR: return "AST_CTOR";
        case AST_NEW: return "AST_NEW";
        case AST_OBJECT: return "AST_OBJECT";
        case AST_SWITCH: return "AST_SWITCH";
        case AST_CASE: return "AST_CASE";
        case AST_DEFAULT: return "AST_DEFAULT";
    }
    return "UNKNOWN_AST_NODE_TYPE";
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
    
    if (node->value.strval &&
        node->type != AST_FN_DECL && 
        node->type != AST_IF && 
        node->type != AST_FOR &&
        node->type != AST_FOREACH &&
        node->type != AST_LIB_CALL &&
        node->type != AST_SWITCH) {
        printf(", Value: \"%s\"", node->value.strval->value);
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

    if (node->type == AST_FN_DECL || 
        node->type == AST_IF || 
        node->type == AST_FOR || 
        node->type == AST_FOREACH ||
        node->type == AST_LIB_CALL ||
        node->type == AST_SWITCH) {
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
AST *create_ast_node(ASTNodeType type, AST *left, AST *right, ASTValue value, bool val_is_ast, SrcLoc srcloc) {
    AST *node = apexMem_alloc(sizeof(AST));
    node->type = type;
    node->left = left;
    node->right = right;
    node->value = value;
    node->val_is_ast = val_is_ast;
    node->srcloc = srcloc;
    return node;
}

/**
 * Creates an error abstract syntax tree node.
 *
 * This function allocates memory for an AST node and initializes it
 * as an error node with the given source location. The node has no
 * children and has a default value of zero.
 *
 * @param srcloc The source location to associate with the error node.
 * @return A pointer to the newly allocated error AST node.
 */

AST *create_error_ast(void) {
    AST *node = apexMem_alloc(sizeof(AST));
    node->type = AST_ERROR;
    node->left = NULL;
    node->right = NULL;
    node->srcloc.filename = NULL;
    node->srcloc.lineno = 0;
    node->val_is_ast = false;
    node->value = ast_value_zero();
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
 * Creates an ASTValue with the given string as the value.
 *
 * @param str The string to associate with the ASTValue.
 *
 * @return An ASTValue with the given string as the value.
 */
ASTValue ast_value_str(ApexString *str) {
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
        if (node->val_is_ast) {
            free_ast(node->value.ast_node);
        }
        free(node);
    }
}
