#include "malloc.h"
#include "ast.h"

#define AST_STACK_INIT_SIZE 256
#define AST_LIST_INIT_SIZE 64

ApexAstNode *apex_ast_node_new(void) {
    ApexAstNode *node = APEX_NEW(ApexAstNode);    

    node->left = NULL;
    node->right = NULL;
    return node;
}

ApexAst *apex_ast_new(void) {
    ApexAst *ast = APEX_NEW(ApexAst);

    ast->size = AST_STACK_INIT_SIZE;
    ast->n = 0;
    ast->nodes = apex_malloc(ast->size * sizeof(ApexAstNode *));
    return ast;
}

void apex_ast_push_node(ApexAst *ast, ApexAstNode *node) {
    ast->nodes[ast->n++] = node;
}

void apex_ast_int(ApexAst *ast, int v) {
    ApexAstNode *node = apex_ast_node_new();

    APEX_VALUE_INT(&node->data.value) = v;
    node->node_type = APEX_AST_NODE_INT;
    apex_ast_push_node(ast, node); 
}

void apex_ast_str(ApexAst *ast, char *v) {
    ApexAstNode *node = apex_ast_node_new();

    APEX_VALUE_STR(&node->data.value) = v;
    node->node_type = APEX_AST_NODE_STR;
    apex_ast_push_node(ast, node); 
}

void apex_ast_id(ApexAst *ast, char *id) {
    ApexAstNode *node = apex_ast_node_new();

    node->data.id = id;
    node->node_type = APEX_AST_NODE_ID;
    apex_ast_push_node(ast, node); 
}

void apex_ast_list(ApexAst *ast, ApexAst *list) {
    ApexAstNode *node = apex_ast_node_new();

    node->node_type = APEX_AST_NODE_LIST;
    node->data.list = list;
    apex_ast_push_node(ast, node);
}

void apex_ast_opr1(ApexAst *ast, ApexAstOpr opr, ApexAstNode *arg) {
    ApexAstNode *node = apex_ast_node_new();

    node->node_type = APEX_AST_NODE_OPR;
    node->value_type = APEX_TYPE_NONE;
    node->data.opr = opr;
    node->left = arg;
    apex_ast_push_node(ast, node); 
}

void apex_ast_opr2(ApexAst *ast, ApexAstOpr opr, ApexAstNode *left, ApexAstNode *right) {
    ApexAstNode *node = apex_ast_node_new();

    node->node_type = APEX_AST_NODE_OPR;
    node->value_type = APEX_TYPE_NONE;
    node->data.opr = opr;
    node->left = left;
    node->right = right;
    apex_ast_push_node(ast, node); 
}   

ApexAstNode *apex_ast_pop(ApexAst *ast) {
    if (ast->n == 0) {
        return NULL;
    }
    return ast->nodes[--ast->n];
}
