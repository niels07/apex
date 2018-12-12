#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error.h"
#include "type.h"
#include "malloc.h"
#include "compiler.h"
#include "vm.h"

#define CODE_INIT_SIZE 5120

#define TOP(c) (*((c)->code->bytes + (c)->code->n))

#define ADD_VALUE(T, cmpl, value) do {      \
    check_code_size((cmpl), sizeof(T));     \
    TOP(cmpl) = (value);                    \
    (cmpl)->code->n += sizeof(T);           \
} while (0)

#define ADD_BYTE(cmpl, v) do {      \
    check_code_size((cmpl), 1);     \
    TOP(cmpl) = (ApexByte)(v);      \
    (cmpl)->code->n++;              \
} while (0)

struct ApexCompiler {
    ApexHashTable *table;
    ApexAst *ast;
    ApexVmCode *code;
    size_t size;
};

static void check_code_size(ApexCompiler *cmpl, size_t size) {
    if (cmpl->code->n + size < cmpl->code->size) {
        return;
    }
    cmpl->code->size *= 2;
    cmpl->code->bytes = apex_realloc(cmpl->code->bytes, cmpl->code->size);
}

static void handle_node(ApexCompiler *, ApexAstNode *);

static void invalid_operation1(const char *opr, ApexAstNode *node) {
    apex_error_type(
        "operation '%s' on '%s' not supported",
        opr,
        apex_type_get_name(node->value_type));
}

static void invalid_operation2(const char *opr, ApexAstNode *left, ApexAstNode *right) {
    apex_error_type(
        "invalid operation '%s' on '%s' and '%s'",
        opr,
        apex_type_get_name(left->value_type),
        apex_type_get_name(right->value_type));
}

static ApexType get_id_type(ApexAstNode *node) {
    return 0;     
}

static void save_code(ApexCompiler *cmpl, const char *output) {
    FILE *file = fopen(output, "wb");

    if (!file) {
        apex_error_io("could not open file: %s", strerror(errno));
    }

    if (fwrite(&cmpl->code->n, sizeof(size_t), 1, file) == 0) {
        fclose(file);
        apex_error_io("could not write to file: %s", strerror(errno));
    }

    if (fwrite(cmpl->code->bytes, cmpl->code->n, 1, file) == 0) {
        fclose(file);
        apex_error_io("could not write to file: %s", strerror(errno));
    }

    fclose(file);
}

static void add_str(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    switch (right->value_type) {
    case APEX_TYPE_STR:
        ADD_BYTE(cmpl, APEX_VM_OP_ADD);
        break;

    default:
        invalid_operation2("+", left, right);
        break;
    }
}

static void add_number(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    switch (right->value_type) {
    case APEX_TYPE_INT:
    case APEX_TYPE_FLT:
        ADD_BYTE(cmpl, APEX_VM_OP_ADD);
        return;

    default:
        invalid_operation2("+", left, right);
        break;
    }
}

static void add(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    handle_node(cmpl, left);
    handle_node(cmpl, right); 

    switch (left->value_type) {
    case APEX_TYPE_STR:
        add_str(cmpl, left, right);
        break;

    case APEX_TYPE_FLT:
    case APEX_TYPE_INT:
        add_number(cmpl, left, right);
        break;

    default:
        invalid_operation1("+", left);
        break;
    }
}

static void sub_number(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    switch (right->value_type) {
    case APEX_TYPE_INT:
    case APEX_TYPE_FLT:
        break;

    default:
        invalid_operation2("-", left, right);
        break;
    }
    ADD_BYTE(cmpl, APEX_VM_OP_SUB);
}

static void sub(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    handle_node(cmpl, left);
    handle_node(cmpl, right); 

    switch (left->value_type) {
    case APEX_TYPE_INT:
    case APEX_TYPE_FLT:
        sub_number(cmpl, left, right);
        break;

    default:
        invalid_operation1("-", left);
        break;
    }
}

static void div_number(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    switch (right->value_type) {
    case APEX_TYPE_INT:
        if (APEX_AST_NODE_INT(right) == 0) {
            apex_error_throw(APEX_ERROR_CODE_TYPE, "division by 0");
        }
        ADD_BYTE(cmpl, APEX_VM_OP_DIV);
        break;

    case APEX_TYPE_FLT:
        if (APEX_AST_NODE_FLT(right) == 0.0) {
            apex_error_throw(APEX_ERROR_CODE_TYPE, "division by 0");
        }
        ADD_BYTE(cmpl, APEX_VM_OP_DIV);
        break;

    default:
        invalid_operation2("/", left, right);
        break;
    }
}

static void div(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    handle_node(cmpl, left);
    handle_node(cmpl, right); 

    switch (left->value_type) {
    case APEX_TYPE_INT:
    case APEX_TYPE_FLT:
        div_number(cmpl, left, right);
        break;

    default:
        invalid_operation1("/", left);
        break;
    }
}

static void mul_number(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    switch (right->value_type) {
    case APEX_TYPE_INT:
    case APEX_TYPE_FLT:
        ADD_BYTE(cmpl, APEX_VM_OP_MUL);
        break;

    default:
        invalid_operation2("-", left, right);
        break;
    }
}

static void mul(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    handle_node(cmpl, left);
    handle_node(cmpl, right); 

    switch (left->value_type) {
    case APEX_TYPE_FLT:
    case APEX_TYPE_INT:
        mul_number(cmpl, left, right);
        break;

    default:
        invalid_operation1("*", left);
        break;
    }
}

static void call(ApexCompiler *cmpl, ApexAstNode *left, ApexAstNode *right) {
    handle_node(cmpl, left);
    handle_node(cmpl, right);
    ADD_BYTE(cmpl, APEX_VM_OP_CALL);
    
    if (left && left->node_type == APEX_AST_NODE_LIST) {
        ADD_VALUE(int, cmpl, APEX_AST_NODE_LIST(left)->n);
    }
}

static void import(ApexCompiler *cmpl, ApexAstNode *left) {
    handle_node(cmpl, left);
    ADD_BYTE(cmpl, APEX_VM_OP_IMPORT);
}

static void opr(ApexCompiler *cmpl, ApexAstNode *node) {
    switch (APEX_AST_NODE_OPR(node)) {
    case '+':
        add(cmpl, node->left, node->right);
        break;

    case '-':
        sub(cmpl, node->left, node->right);
        break;

    case '*':
        mul(cmpl, node->left, node->right);
        break;

    case '/':
        div(cmpl, node->left, node->right);
        break;

    case APEX_AST_OPR_IMPORT:
        import(cmpl, node->left);
        break;

    case APEX_AST_OPR_CALL:
        call(cmpl, node->left, node->right);
        break;
    }
}

static void handle_node(ApexCompiler *cmpl, ApexAstNode *node) {
    switch (node->node_type) {
    case APEX_AST_NODE_OPR:
        opr(cmpl, node);
        node->value_type = node->left->value_type;
        break;

    case APEX_AST_NODE_INT:
        node->value_type = APEX_TYPE_INT;
        ADD_BYTE(cmpl, APEX_VM_OP_PUSH_INT);
        ADD_VALUE(int, cmpl, APEX_AST_NODE_INT(node));
        break;

    case APEX_AST_NODE_FLT:
        node->value_type = APEX_TYPE_FLT;
        ADD_VALUE(float, cmpl, APEX_AST_NODE_FLT(node));
        break;

    case APEX_AST_NODE_STR: {
        char *str = APEX_AST_NODE_STR(node);
        size_t len = strlen(str) + 1;

        node->value_type = APEX_TYPE_STR;
        ADD_BYTE(cmpl, APEX_VM_OP_PUSH_STR);
        check_code_size(cmpl, len);
        memcpy(&TOP(cmpl), str, len);
        cmpl->code->n += len;
        break;
    }

    case APEX_AST_NODE_ID: {
        char *str = APEX_AST_NODE_ID(node);
        size_t len = strlen(str) + 1;

        ADD_BYTE(cmpl, APEX_VM_OP_LOAD_REF);
        node->value_type = get_id_type(node);
        check_code_size(cmpl, len);
        memcpy(&TOP(cmpl), str, len);
        cmpl->code->n += len;
        break;
    }
    
    case APEX_AST_NODE_LIST: { 
        ApexAst *ast = APEX_AST_NODE_LIST(node);
        size_t i;

        cmpl->size += ast->n;
        for (i = 0; i < ast->n; i++) {
            ApexAstNode *node = ast->nodes[i];
            handle_node(cmpl, node);
        }
        break;
    }

    default:
        apex_error_internal("unkown node type: %d", node->node_type);
        break;
    }
}

ApexCompiler *apex_compiler_new(ApexAst *ast, ApexHashTable *table) {
    ApexCompiler *cmpl = APEX_NEW(ApexCompiler);

    cmpl->table = table;
    cmpl->ast = ast;
    cmpl->code = APEX_NEW(ApexVmCode);
    cmpl->code->size = CODE_INIT_SIZE;
    cmpl->code->n = 0;
    cmpl->code->bytes = apex_malloc(CODE_INIT_SIZE);
    return cmpl;
}

void apex_compiler_destroy(ApexCompiler *cmpl) {
    apex_free(cmpl->code);
    apex_free(cmpl->code->bytes);
    apex_free(cmpl);
}

void apex_compiler_compile(ApexCompiler *cmpl, const char *output) {
    size_t i;

    cmpl->size = cmpl->ast->n;
    for (i = 0; i < cmpl->ast->n;  i++) {
        ApexAstNode *node = cmpl->ast->nodes[i];
        handle_node(cmpl, node);
    }
    ADD_BYTE(cmpl, APEX_VM_OP_END);
    save_code(cmpl, output);
}

