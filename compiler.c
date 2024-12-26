#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "vm.h"
#include "value.h"
#include "error.h"
#include "symbol.h"
#include "bool.h"
#include "mem.h"
#include "string.h"
#include "stdlib.h"

static const char *get_ast_node_type_name(ASTNodeType type) {
    switch (type) {
        case AST_ERROR: return "AST_ERROR";
        case AST_INT: return "AST_INT";
        case AST_FLT: return "AST_FLT";
        case AST_BOOL: return "AST_BOOL";
        case AST_STR: return "AST_STR";
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
        case AST_CONTINUE: return "AST_CONTINUE";
        case AST_BREAK: return "AST_BREAK";
        default: return "UNKNOWN_AST_NODE_TYPE";
    }
}

#define EMIT_OP_INT(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makeint(value))
#define EMIT_OP_FLT(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makeflt(value))
#define EMIT_OP_STR(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makestr(value))
#define EMIT_OP_BOOL(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makebool(value))
#define EMIT_OP(vm, opcode) emit_instruction(vm, opcode, zero_value())
#define UPDATE_SRCLOC(vm, node) vm->srcloc = node->srcloc

static void compile_expression(ApexVM *vm, AST *node);
static void compile_statement(ApexVM *vm, AST *node);

/**
 * Ensures that the instruction buffer has enough capacity to store one more
 * instruction.
 */
static void ensure_capacity(ApexVM *vm) {
    if (vm->chunk->ins_n + 1 >= vm->chunk->ins_size) {
        vm->chunk->ins_size = vm->chunk->ins_size * 2;
        vm->chunk->ins = mem_realloc(
            vm->chunk->ins, 
            sizeof(Ins) * vm->chunk->ins_size);
    }
}

/**
 * Returns a Value with all fields set to zero, which can be used as a null
 * or placeholder value.
 */
static ApexValue zero_value(void) {
    ApexValue value;
    memset(&value, 0, sizeof(ApexValue));
    return value;
}

/**
 * Emits an instruction to the virtual machine's instruction chunk.
 *
 * This function ensures that there is enough capacity in the instruction
 * buffer to store a new instruction before proceeding. It logs the process of
 * emitting the instruction for debugging purposes, printing the opcode and
 * associated value to the console. Depending on the opcode, it prints the
 * integer, float, string, or boolean value being emitted.
 *
 * The function stores the provided opcode and value into the next available
 * slot in the ApexVM's instruction chunk, incrementing the instruction count
 * afterwards.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param opcode The opcode representing the operation to be emitted.
 * @param value The value to be associated with the opcode in the instruction.
 */
static void emit_instruction(ApexVM *vm, OpCode opcode, ApexValue value) {
    ensure_capacity(vm);
    vm->chunk->ins[vm->chunk->ins_n].srcloc = vm->srcloc;
    vm->chunk->ins[vm->chunk->ins_n].value = value;
    vm->chunk->ins[vm->chunk->ins_n].opcode = opcode;
    vm->chunk->ins_n++;
}

/**
 * Compiles an AST node representing a variable to bytecode.
 *
 * If the variable is found in the local or global symbol table, it emits an
 * instruction to either get or set the variable, depending on the value of
 * is_assignment.
 *
 * If the variable is not found in either symbol table, and is_assignment is true,
 * it adds the variable to the local or global symbol table, depending on whether
 * or not the ApexVM is currently inside a function. It then emits an instruction to
 * set the variable.
 *
 * If the variable is not found and is_assignment is false, it emits an error
 * message and does not emit any bytecode.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the local and global symbol tables.
 * @param node The AST node representing the variable to be compiled.
 * @param is_assignment A boolean indicating whether or not this variable is
 *                      being assigned to. If true, an instruction to set the
 *                      variable is emitted. If false, an instruction to get the
 *                      variable is emitted.
 */
static void compile_variable(ApexVM *vm, AST *node, Bool is_assignment) {
    int addr;
    char *var_name = node->value.strval;

    if (node->type != AST_VAR) {
        return;
    }

    addr = get_local_symbol_address(&vm->local_scopes, var_name);

    if (addr != -1) {
        if (is_assignment) {
            EMIT_OP_INT(vm, OP_SET_LOCAL, addr);
        } else {
            EMIT_OP_INT(vm, OP_GET_LOCAL, addr);
        }
        return;
    }

    addr = get_symbol_address(&vm->global_table, var_name);

    if (addr != -1) {
        if (is_assignment) {
            EMIT_OP_STR(vm, OP_SET_GLOBAL, var_name);
        } else {
            EMIT_OP_STR(vm, OP_GET_GLOBAL, var_name);
        }
        return;
    }

    if (is_assignment) {
        if (vm->in_function) {
            addr = add_local_symbol(&vm->local_scopes, var_name);
            EMIT_OP_INT(vm, OP_SET_LOCAL, addr);
        } else {
            add_symbol(&vm->global_table, var_name);
            EMIT_OP_STR(vm, OP_SET_GLOBAL, var_name);
        }
    } else {
        apexErr_fatal(node->srcloc, "variable '%s' is not defined", var_name);
    }
}

/**
 * Compiles an AST node representing a parameter list to bytecode.
 *
 * This function iterates through the parameter list and adds each parameter to
 * the local symbol table, assigning it a unique index.
 *
 * If the parameter list is empty, or if any of the parameters are not variables
 * (AST_VAR), an error message is reported.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the local and global symbol tables.
 * @param parameter_list The AST node representing the parameter list to be
 *                       compiled.
 */
static const char **compile_parameter_list(ApexVM *vm, AST *param_list, int *param_n) {
    int pcount = 0;
    int psize = 8;
    const char **params = mem_alloc(sizeof(char *) * psize);

    while (param_list) {
         if (param_list->type == AST_BLOCK) {
            break;
        } 
        if (param_list->type == AST_VAR) {
            const char *param_name = param_list->value.strval;

            add_local_symbol(&vm->local_scopes, param_name);
            if (pcount >= psize) {
                psize *= 2;
                params = mem_realloc(params, sizeof(char *) * psize);
            }
            params[pcount++] = param_name;
            break;
        } else if (param_list->type == AST_PARAMETER_LIST) {
            AST *parameter = param_list->left;
            const char *param_name = parameter->value.strval;

            if (parameter->type != AST_VAR) {
                apexErr_fatal(
                    parameter->srcloc,
                    "expected parameter to be a variable, got: %s", 
                    get_ast_node_type_name(parameter->type));
            }
            add_local_symbol(&vm->local_scopes, parameter->value.strval);
            if (pcount >= psize) {
                psize *= 2;
                params = mem_realloc(params, sizeof(char *) * psize);
            }
            params[pcount++] = param_name;

            if (param_list->right && 
               (param_list->right->type == AST_PARAMETER_LIST ||
                param_list->right->type == AST_VAR)) {
                param_list = param_list->right;
            } else {
                break;
            }
        } else {
            apexErr_fatal(
                param_list->srcloc,
                "Invalid AST node in parameter list, expected "
                "AST_PARAMETER_LIST or AST_VARIABLE, got %s", 
                get_ast_node_type_name(param_list->type));
        }
    }
    *param_n = pcount;
    return params;
}

/**
 * Compiles an AST node representing an argument list to bytecode.
 *
 * This function recursively traverses the argument list, compiling each
 * argument expression in the list. It processes arguments from left
 * to right, ensuring that they are evaluated in the correct order.
 *
 * If the argument list is empty, the function returns immediately.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param argument_list The AST node representing the argument list to be
 *                      compiled.
 */
static void compile_argument_list(ApexVM *vm, AST *argument_list) {
    if (argument_list == NULL) {
        return;
    }
    compile_argument_list(vm, argument_list->left);
    compile_expression(vm, argument_list->right);
}

/**
 * Compiles an AST node representing a function declaration to bytecode.
 *
 * This function compiles the parameter list and the function body, and adds
 * the function to the function table. The function name is set as the key in
 * the function table, and the address of the function in the instruction chunk
 * is stored as the value.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the function table.
 * @param node The AST node representing the function declaration to be
 *             compiled.
 */
static void compile_function_declaration(ApexVM *vm, AST *node) {
    const char *fn_name = node->left->value.strval;
    int param_n = 0;
    const char **params;
    Fn *fn;

    vm->in_function = TRUE;

    EMIT_OP(vm, OP_FUNCTION_START);

    push_scope(&vm->local_scopes);

    params = compile_parameter_list(vm, node->value.ast_node, &param_n);
    fn = mem_alloc(sizeof(Fn));
    fn->name = fn_name;
    fn->param_n = param_n;
    fn->params = params;
    fn->addr = vm->chunk->ins_n;

    set_symbol_value(&vm->fn_table, fn_name, apexVal_makefn(fn));

    compile_statement(vm, node->right);
    EMIT_OP(vm, OP_RETURN);
    pop_scope(&vm->local_scopes);
    EMIT_OP(vm, OP_FUNCTION_END);

    vm->in_function = FALSE;
}

/**
 * Compiles an AST node representing a function call to bytecode.
 *
 * This function compiles the arguments and the function name, and adds the
 * function call to the instruction chunk. The function name is looked up in
 * the function table and the address of the function in the instruction chunk
 * is stored as the value.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the function table.
 * @param node The AST node representing the function call to be compiled.
 */
static void compile_function_call(ApexVM *vm, AST *node) {
    const char *fn_name = node->left->value.strval;
    int fn_addr;
    int i;

    UPDATE_SRCLOC(vm, node);

    for (i = 0; apex_stdlib[i].name; i++) {
        if (strcmp(apex_stdlib[i].name, fn_name) == 0) {
            compile_argument_list(vm, node->right);
            EMIT_OP_INT(vm, OP_CALL_LIB, i);
            return;
        }
    }

    fn_addr = get_symbol_address(&vm->fn_table, fn_name);
    
    if (fn_addr == -1) {
        apexErr_fatal(node->srcloc, "function '%s' is not defined", fn_name);
    }

    compile_argument_list(vm, node->right);
    EMIT_OP_INT(vm, OP_CALL, fn_addr);
}

/**
 * Compiles an AST node representing an expression to bytecode.
 *
 * This function compiles the AST node recursively, emitting instructions to
 * the instruction chunk as it goes. It handles all expression types, including
 * binary expressions, unary expressions, logical expressions, variables, and
 * function calls. It also handles assignment and constant expressions.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the expression to be compiled.
 */
static void compile_expression(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    printf("[DEBUG] Compiling expression: %s\n", get_ast_node_type_name(node->type));
    switch (node->type) {
    case AST_INT:
        EMIT_OP_INT(vm, OP_PUSH_INT, atoi(node->value.strval));
        break;

    case AST_FLT:
        EMIT_OP_FLT(vm, OP_PUSH_FLT, atof(node->value.strval));
        break;

    case AST_STR:
        EMIT_OP_STR(vm, OP_PUSH_STR, node->value.strval);
        break;

    case AST_BOOL:
        EMIT_OP_BOOL(vm, OP_PUSH_BOOL, strcmp(node->value.strval, "true") == 0 ? TRUE : FALSE);
        break;
    
    case AST_BINARY_EXPR:
        compile_expression(vm, node->left);
        compile_expression(vm, node->right);
        if (strcmp(node->value.strval, "+") == 0) {
            EMIT_OP(vm, OP_ADD);
        } else if (node->value.strval == new_string("+", 1)) {
            EMIT_OP(vm, OP_ADD);
        } else if (node->value.strval == new_string("-", 1)) {
            EMIT_OP(vm, OP_SUB);
        } else if (node->value.strval == new_string("*", 1)) {
            EMIT_OP(vm, OP_MUL);
        } else if (node->value.strval == new_string("/", 1)) {
            EMIT_OP(vm, OP_DIV);
        } else if (node->value.strval == new_string("==", 2)) {
            EMIT_OP(vm, OP_EQ);
        } else if (node->value.strval == new_string("!=", 2)) {
            EMIT_OP(vm, OP_NE);
        } else if (node->value.strval == new_string("<", 1)) {
            EMIT_OP(vm, OP_LT);
        } else if (node->value.strval == new_string("<=", 2)) {
            EMIT_OP(vm, OP_LE);
        } else if (node->value.strval == new_string(">", 1)) {
            EMIT_OP(vm, OP_GT);
        } else if (node->value.strval == new_string(">=", 2)) {
            EMIT_OP(vm, OP_GE);
        }
        break;

    case AST_UNARY_EXPR:
        compile_expression(vm, node->right);
        if (strcmp(node->value.strval, "!") == 0){
            EMIT_OP(vm, OP_NOT);
        }
        if (strcmp(node->value.strval, "-") == 0) {
            EMIT_OP(vm, OP_NEGATE);
        }
        break;

    case AST_LOGICAL_EXPR: {
        int short_circuit_jmp;
        int end_jmp;
        compile_expression(vm, node->left);

        if (node->value.strval == new_string("&&", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_n - 1;
            compile_expression(vm, node->right);
            EMIT_OP(vm, OP_JUMP);
            end_jmp = vm->chunk->ins_n - 1;
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_n - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, FALSE);
            vm->chunk->ins[end_jmp].value.intval = vm->chunk->ins_n - end_jmp - 1;
        } else if (node->value.strval == new_string("||", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_n - 1;
            compile_expression(vm, node->right);
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_n - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, TRUE);
        }
        break;
        
    }
    case AST_VAR:
        compile_variable(vm, node, FALSE);
        break;

    case AST_ASSIGNMENT:
        compile_expression(vm, node->right);
        compile_variable(vm, node->left, TRUE);
        break;

    case AST_FN_CALL: 
        compile_function_call(vm, node);
        break;

    default:
        apexErr_fatal(node->srcloc, "Unhandled AST node type: %d", node->type);
    }
}

/**
 * Compiles an AST node representing a loop to bytecode.
 *
 * This function compiles the loop condition, body, and increment (if they exist),
 * and adds the loop to the instruction chunk. The start and end of the loop are
 * stored as the loop_start and loop_end fields of the ApexVM, respectively. After
 * compiling the loop, the values of loop_start and loop_end are restored to their
 * previous values.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk, the loop start and end indices, and the
 *           function table.
 * @param condition The AST node representing the loop condition to be
 *                  compiled.
 * @param body The AST node representing the loop body to be compiled.
 * @param increment The AST node representing the loop increment to be
 *                  compiled.
 */
static void compile_loop(ApexVM *vm, AST *condition, AST *body, AST *increment) {
    int previous_loop_start = vm->loop_start;
    int previous_loop_end = vm->loop_end;

    UPDATE_SRCLOC(vm, condition);
    vm->loop_start = vm->chunk->ins_n;

    if (condition) {
        compile_expression(vm, condition);
        EMIT_OP(vm, OP_JUMP_IF_FALSE);
        vm->loop_end = vm->chunk->ins_n - 1;
    } else {
        vm->loop_end = -1;
    }

    compile_statement(vm, body);

    if (increment) {
        compile_statement(vm, increment);
    }

    EMIT_OP_INT(vm, OP_JUMP, vm->loop_start - vm->chunk->ins_n - 1);

    if (condition) {
        vm->chunk->ins[vm->loop_end].value.intval = vm->chunk->ins_n - vm->loop_end - 1;
    }

    vm->loop_start = previous_loop_start;
    vm->loop_end = previous_loop_end;
}

/**
 * Compiles an AST node representing a statement to bytecode.
 *
 * This function recursively traverses the AST statement structure, compiling
 * each sub-statement in the structure. It processes statements from top to
 * bottom, ensuring that they are evaluated in the correct order.
 *
 * If the statement is empty, the function returns immediately.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the statement to be compiled.
 */
static void compile_statement(ApexVM *vm, AST *node) {
    if (!node) {
        return;
    }
    printf("[DEBUG] Compiling statement: %s - %d\n", get_ast_node_type_name(node->type), node->type); 
    switch (node->type) {
    case AST_IF: {
        int false_jmp_i;
        int true_jmp_i;

        UPDATE_SRCLOC(vm, node);

        compile_expression(vm, node->left); /* Condition */
        EMIT_OP(vm, OP_JUMP_IF_FALSE);
        false_jmp_i = vm->chunk->ins_n - 1;

        compile_statement(vm, node->right); /* Block or statement */
        EMIT_OP(vm, OP_JUMP);
        true_jmp_i = vm->chunk->ins_n - 1;

        if (node->value.ast_node) {
            vm->chunk->ins[false_jmp_i].value.intval = vm->chunk->ins_n - false_jmp_i - 1;
            compile_statement(vm, node->value.ast_node);
        } else {
            vm->chunk->ins[false_jmp_i].value.intval = vm->chunk->ins_n - false_jmp_i - 1;
        }
        vm->chunk->ins[true_jmp_i].value.intval = vm->chunk->ins_n - true_jmp_i - 1;
        break;
    }
    case AST_WHILE: 
        compile_loop(vm, node->left, node->right, NULL);
        break;

    case AST_FOR: 
        compile_statement(vm, node->left);
        compile_loop(vm, node->right->left, node->right->right, node->right->left);
        break;
        
    case AST_CONTINUE:
        if (vm->loop_start == -1) {
            apexErr_fatal(node->srcloc, "Invalid 'continue' outside of loop on line %d");
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_start - vm->chunk->ins_n - 1);
        break;

    case AST_BREAK:
        if (vm->loop_end == -1) {
            apexErr_fatal(node->srcloc,"Invalid 'break' outside of loop on line %d");
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_end - vm->chunk->ins_n - 1);
        break;

    case AST_FN_DECL:
        compile_function_declaration(vm, node);
        break;

    case AST_RETURN:
        compile_expression(vm, node->left);
        EMIT_OP(vm, OP_RETURN);
        break;

    case AST_BLOCK:            
        compile_statement(vm, node->right);
        if (node->left) {
            compile_statement(vm, node->left);
        }           
        break;

    case AST_STATEMENT:
        if (node->left) {
            compile_statement(vm, node->left);
        }
        if (node->right) {
            compile_statement(vm, node->right);
        }
        break;

    default:
        compile_expression(vm, node);
    }
}

/**
 * Predeclare symbols in the program.
 *
 * This function predeclares all symbols in the program, by iterating through
 * the AST and adding each symbol to the global or function symbol table, as
 * appropriate.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the local and global symbol tables.
 * @param node The AST node representing the statement to be compiled.
 * @param in_function A boolean indicating whether or not this statement is
 *                    currently inside a function. If true, the function symbol
 *                    table is used. If false, the global symbol table is used.
 */
static void predeclare_symbols(ApexVM *vm, AST *node, Bool in_function) {
    Bool have_function = FALSE;

    while (node != NULL) {
        if (node->type == AST_FN_DECL) {
            add_symbol(&vm->fn_table, node->left->value.strval);
            predeclare_symbols(vm, node->right, TRUE);
            have_function = TRUE;
        } else if (node->type == AST_ASSIGNMENT && !in_function) {
            add_symbol(&vm->global_table, node->left->value.strval);
        }

        if (node->left) {
            predeclare_symbols(vm, node->left, in_function);
        }
        if (node->right && !have_function) {
            predeclare_symbols(vm, node->right, in_function);
        }
        break;
    }
}

/**
 * Compile an AST to bytecode.
 *
 * This function predeclares all symbols in the program, and then compiles the
 * program to bytecode. The resulting bytecode is stored in the instruction
 * chunk in the virtual machine.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and the local and global symbol tables.
 * @param program The AST node representing the program to be compiled.
 */
void compile(ApexVM *vm, AST *program) {
    predeclare_symbols(vm, program, FALSE);
    if (program->left) {
        compile_statement(vm, program->left);
    }
    
    if (program->right) {
        compile_statement(vm, program->right);
    }    
    EMIT_OP(vm, OP_HALT);
}
