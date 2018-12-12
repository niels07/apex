#ifndef APEX_AST_H
#define APEX_AST_H

#include <stddef.h>
#include <apex/value.h>

typedef enum {
    APEX_AST_OPR_GE,
    APEX_AST_OPR_LE,
    APEX_AST_OPR_EQ,
    APEX_AST_OPR_NE,
    APEX_AST_OPR_AND,
    APEX_AST_OPR_CALL,
    APEX_AST_OPR_IMPORT
} ApexAstOpr;

typedef enum {
    APEX_AST_NODE_NONE,
    APEX_AST_NODE_INT,
    APEX_AST_NODE_FLT,
    APEX_AST_NODE_STR,
    APEX_AST_NODE_OPR,
    APEX_AST_NODE_ID,
    APEX_AST_NODE_LIST
} ApexAstNodeType;

typedef struct ApexAstNode ApexAstNode;
typedef struct ApexAst ApexAst;
typedef struct ApexAstList ApexAstList;

struct ApexAstNode {
    ApexAstNode *left;
    ApexAstNode *right;
    ApexAstNodeType node_type;
    ApexType value_type;
    union {
        ApexAstOpr opr;
        ApexValue value;
        ApexAst *list;
        char *id;
    } data;
};

struct ApexAst {
    ApexAstNode **top;
    ApexAstNode **nodes;
    size_t size;
    size_t n;
};

extern ApexAst *apex_ast_new(void);

extern void apex_ast_opr1(ApexAst *, ApexAstOpr, ApexAstNode *);
extern void apex_ast_opr2(ApexAst *, ApexAstOpr, ApexAstNode *, ApexAstNode *);
extern void apex_ast_int(ApexAst *, int);
extern void apex_ast_str(ApexAst *, char *);
extern void apex_ast_list(ApexAst *, ApexAst *);
extern void apex_ast_id(ApexAst *, char *);

extern ApexAstNode *apex_ast_pop(ApexAst *);
extern ApexAstNode *apex_ast_node_new(void);

#define APEX_AST_NODE_INT(node) ((node)->data.value.data.intval)
#define APEX_AST_NODE_FLT(node) ((node)->data.value.data.fltval)
#define APEX_AST_NODE_STR(node) ((node)->data.value.data.strval)
#define APEX_AST_NODE_LIST(node) ((node)->data.list)
#define APEX_AST_NODE_OPR(node) ((node)->data.opr)
#define APEX_AST_NODE_ID(node) ((node)->data.id)

#endif
