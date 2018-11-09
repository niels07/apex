#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "io.h"
#include "lexer.h"

static void print_node_data(ApexAstNode *node, char *type) {
    printf("%s: ", type);
    switch (node->node_type) {
    case APEX_AST_NODE_NONE:
        break;

    case APEX_AST_NODE_INT:
        printf("%d ", APEX_AST_NODE_INT(node));
        break;

    case APEX_AST_NODE_FLT:
        printf("%f ", APEX_AST_NODE_FLT(node));
        break;

    case APEX_AST_NODE_STR:
        printf("%s ", APEX_AST_NODE_STR(node));
        break;

    case APEX_AST_NODE_OPR:
        switch (APEX_AST_NODE_OPR(node)) {
        case APEX_AST_OPR_GE:
            printf(">=");
            break;

        case APEX_AST_OPR_LE:
            printf(">=");
            break;
        case APEX_AST_OPR_EQ:
            printf("==");
            break;

        case APEX_AST_OPR_NE:
            printf("!=");
            break;

        case APEX_AST_OPR_AND:
            printf("&&");
            break;

        case APEX_AST_OPR_PRINT:
            printf("print");
            break;

        default:
            printf("%c", APEX_AST_NODE_OPR(node));
            break;
        }
        break;

    case APEX_AST_NODE_ID:
        printf("%s", APEX_AST_NODE_ID(node));
        break;
    }
}

static void print_node(ApexAstNode *node, int space, char *type) {
    int i;
    space += 10; 
    if (!node) {
        return;
    } 
     
    print_node(node->right, space, "right"); 
    putchar('\n'); 
    for (i = 10; i < space; i++) {
        putchar(' '); 
    }
    print_node_data(node, type);
    putchar('\n'); 
    print_node(node->left, space, "left"); 
}
  
int main(void) {
    int i;
    ApexAstNode *node;
    ApexStream *stream = apex_stream_open("test.apex");
    ApexLexer *lexer = apex_lexer_new(stream);
    ApexAst *ast = apex_ast_new();
    ApexParser *parser = apex_parser_new(lexer, ast);
    apex_parser_parse(parser);
    
    for (i = 0; i < ast->n; i++) {
        node = ast->nodes[i];
        print_node(node, 0, "root");
    } 
    return 0;
}

