#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "compiler.h"
#include "vm.h"
#include "value.h"
#include "error.h"
#include "symbol.h"
#include "mem.h"
#include "string.h"
#include "stdlib.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

#define EMIT_OP_INT(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makeint(value))
#define EMIT_OP_DBL(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makedbl(value))
#define EMIT_OP_STR(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makestr(value))
#define EMIT_OP_BOOL(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makebool(value))
#define EMIT_OP(vm, opcode) emit_instruction(vm, opcode, zero_value())
#define UPDATE_SRCLOC(vm, node) vm->srcloc = node->srcloc

static void compile_expression(ApexVM *vm, AST *node, bool use_result);
static void compile_statement(ApexVM *vm, AST *node);
static void compile_member_access(ApexVM *vm, AST *node, bool is_assignment);
static void compile_object_literal(ApexVM *vm, AST *node);

/**
 * Ensures that the instruction buffer has enough capacity to store one more
 * instruction.
 */
static void ensure_capacity(ApexVM *vm) {
    if (vm->chunk->ins_n + 1 >= vm->chunk->ins_size) {
        vm->chunk->ins_size = vm->chunk->ins_size * 2;
        vm->chunk->ins = apexMem_realloc(
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
 * This function handles local and global variable accesses, depending on
 * whether the virtual machine is currently inside a function or not. If the
 * is_assignment flag is set, it emits an OP_SET_LOCAL or OP_SET_GLOBAL
 * instruction to store a value in the variable. If the flag is not set, it
 * emits an OP_GET_LOCAL or OP_GET_GLOBAL instruction to retrieve the value
 * of the variable.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the variable to be compiled.
 * @param is_assignment A boolean indicating whether the access is an
 *                      assignment. If true, an instruction to set the variable
 *                      is emitted. If false, an instruction to get the variable
 *                      is emitted.
 */
static void compile_variable(ApexVM *vm, AST *node, bool is_assignment) {
    ApexString *var_name = node->value.strval;

    if (node->type != AST_VAR) {
        return;
    }

    if (is_assignment) {
        if (vm->in_function) {            
            EMIT_OP_STR(vm, OP_SET_LOCAL, var_name);
        } else {
            EMIT_OP_STR(vm, OP_SET_GLOBAL, var_name);
        }
    } else {
        if (vm->in_function) {            
            EMIT_OP_STR(vm, OP_GET_LOCAL, var_name);
        } else {
            EMIT_OP_STR(vm, OP_GET_GLOBAL, var_name);
        }
    }
}

/**
 * Compiles an AST node representing a parameter list to a null-terminated
 * array of parameter names.
 *
 * This function iterates over the parameter list, creating a null-terminated
 * array of parameter names. The length of the array is returned in the
 * param_n parameter.
 *
 * @param param_list The AST node representing the parameter list to be
 *                   compiled.
 * @param param_n A pointer to an integer that will be set to the length of
 *                the parameter array.
 * @return A null-terminated array of parameter names.
 */
static const char **compile_parameter_list(AST *param_list, int *param_n) {
    int pcount = 0;
    int psize = 8;
    const char **params = apexMem_alloc(sizeof(char *) * psize);

    while (param_list) {
         if (param_list->type == AST_BLOCK) {
            break;
        } 
        if (param_list->type == AST_VAR) {
            const char *param_name = param_list->value.strval->value;
            if (pcount >= psize) {
                psize *= 2;
                params = apexMem_realloc(params, sizeof(char *) * psize);
            }
            params[pcount++] = param_name;
            break;
        } else if (param_list->type == AST_PARAMETER_LIST) {
            AST *parameter = param_list->left;
            const char *param_name = parameter->value.strval->value;
            if (parameter->type != AST_VAR) {
                apexErr_fatal(parameter->srcloc, "expected parameter to be a variable");
            }
            if (pcount >= psize) {
                psize *= 2;
                params = apexMem_realloc(params, sizeof(char *) * psize);
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
            apexErr_fatal(param_list->srcloc,"Invalid AST node in parameter list");
        }
    }
    *param_n = pcount;
    return params;
}

/**
 * Compiles an AST node representing an array to bytecode.
 *
 * This function iterates through the elements of the array, compiling each
 * expression and emitting an instruction to create a key-value pair or
 * element in the array. It then emits an instruction to create the array with
 * the correct element count.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the array to be compiled.
 */
static void compile_array(ApexVM *vm, AST *node) {
    int n = 0;
    UPDATE_SRCLOC(vm, node);

    AST *current = node->right;
    while (current) {
        if (current->type == AST_KEY_VALUE_PAIR) {
            compile_expression(vm, current->left, true); // key
            compile_expression(vm, current->right, true); // value
            n++;
        } else if (!current->right || current->right->type != AST_KEY_VALUE_PAIR) {
            EMIT_OP_INT(vm, OP_PUSH_INT, n); 
            compile_expression(vm, current, true); // array element
            n++;
        }
        
        current = current->right;
    }

    EMIT_OP_INT(vm, OP_CREATE_ARRAY, n);
}

/**
 * Compiles an AST node representing an array access to bytecode.
 *
 * This function compiles the array and index expressions, and emits the
 * appropriate bytecode instruction to either get or set an element in the
 * array, depending on the value of is_assignment.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the array access to be compiled.
 * @param is_assignment A boolean indicating whether the access is an
 *                      assignment. If true, an instruction to set the element
 *                      is emitted. If false, an instruction to get the element
 *                      is emitted.
 */
static void compile_array_access(ApexVM *vm, AST *node, bool is_assignment) {
    UPDATE_SRCLOC(vm, node);

    compile_expression(vm, node->left, true); // array
    compile_expression(vm, node->right, true); // index

    if (is_assignment) {
        EMIT_OP(vm, OP_SET_ELEMENT);
    } else {
        EMIT_OP(vm, OP_GET_ELEMENT);
    }
}

/**
 * Recursively compiles an argument list to bytecode.
 *
 * This function compiles each argument expression in the list, and
 * increments the argument count for each one.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param argument_list The AST node representing the argument list to be
 *                      compiled.
 * @param argc A pointer to an integer to store the total number of
 *             arguments in the list.
 */
static void compile_argument_list(ApexVM *vm, AST *argument_list, int *argc) {
    if (argument_list == NULL) {
        return;
    }
    compile_argument_list(vm, argument_list->left, argc);
    compile_expression(vm, argument_list->right, true);
    if (argc) {
        (*argc)++;
    }
}

/**
 * Compiles an AST node representing a function declaration to bytecode.
 *
 * This function handles both member and non-member function declarations.
 * It initializes the function scope, compiles the parameter list, and
 * emits the necessary bytecode instructions to define the function.
 * For member functions, it associates the function with an object.
 * The function body is compiled to bytecode, and the function is
 * registered in the global symbol table. After the function is compiled,
 * the scope is popped and the function is finalized.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the function declaration to be
 *             compiled.
 */
static void compile_function_declaration(ApexVM *vm, AST *node) {
    if (node->left->type == AST_MEMBER_FN) {        
        const char *objname = node->left->left->value.strval->value;
        const char *fnname = node->left->right->value.strval->value;
        
        vm->in_function = true;
        EMIT_OP(vm, OP_FUNCTION_START);
        push_scope(&vm->local_scopes);
        int param_n = 0;
        const char **params = compile_parameter_list(node->value.ast_node, &param_n);
        Fn *fn = apexVal_newfn(fnname, params, param_n, vm->chunk->ins_n);
        ApexValue value;
        if (!apexSym_getglobal(&value, &vm->global_table, objname)) {
            apexErr_name(node->srcloc, "object %s not found", objname);
        }
        if (value.type != APEX_VAL_TYPE) {
            apexErr_name(node->srcloc, "%s is not an object", objname);
        }
        ApexObject *obj = value.objval;
        apexVal_objectset(obj, fnname, apexVal_makefn(fn));
        compile_statement(vm, node->right); // function body
        EMIT_OP(vm, OP_RETURN);
        pop_scope(&vm->local_scopes);
        EMIT_OP(vm, OP_FUNCTION_END);
        vm->in_function = false;
    } else {
        const char *fnname = node->left->value.strval->value;
        int param_n = 0;
        vm->in_function = true;
        EMIT_OP(vm, OP_FUNCTION_START);
        push_scope(&vm->local_scopes);
        const char **params = compile_parameter_list(node->value.ast_node, &param_n);
        Fn *fn = apexMem_alloc(sizeof(Fn));
        fn->name = fnname;
        fn->param_n = param_n;
        fn->params = params;
        fn->addr = vm->chunk->ins_n;
        fn->refcount = 0;
        apexSym_setglobal(&vm->global_table, fnname, apexVal_makefn(fn));
        compile_statement(vm, node->right); // function body
        EMIT_OP(vm, OP_RETURN);
        pop_scope(&vm->local_scopes);
        EMIT_OP(vm, OP_FUNCTION_END);
        vm->in_function = false;
    }
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
    ApexString *fn_name = node->left->value.strval;
    int argc = 0;
    UPDATE_SRCLOC(vm, node);
    compile_argument_list(vm, node->right, &argc);
    EMIT_OP_STR(vm, OP_GET_GLOBAL, fn_name);
    EMIT_OP_INT(vm, OP_CALL, argc);
}

/**
 * Compiles an AST node representing a library function call to bytecode.
 *
 * This function compiles the arguments and the library and function names,
 * and adds the library call to the instruction chunk.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the library call to be compiled.
 */
static void compile_library_call(ApexVM *vm, AST *node) {
    int argc = 0;
    compile_argument_list(vm, node->value.ast_node, &argc);

    EMIT_OP_STR(vm, OP_PUSH_STR, node->left->value.strval); // Library name
    EMIT_OP_STR(vm, OP_PUSH_STR, node->right->value.strval);       // Function name
    EMIT_OP_INT(vm, OP_CALL_LIB, argc);
}

/**
 * Compiles an AST node representing an assignment to bytecode.
 *
 * This function handles assignments to variables and array elements. For array
 * assignments, it compiles the array access or array creation as needed. It then
 * compiles the right-hand side expression of the assignment and emits the
 * appropriate bytecode to store the result in the target variable or array element.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the assignment expression to be compiled.
 */
static void compile_assignment(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);

    if (node->left->type == AST_ARRAY_ACCESS) { 
        compile_expression(vm, node->right, true);       
        compile_array_access(vm, node->left, true);        
    } else if (node->right->type == AST_ARRAY) {
        compile_array(vm, node->right);
        compile_variable(vm, node->left, true);
    } else if (node->left->type == AST_MEMBER_ACCESS) {
        compile_expression(vm, node->right, true);       
        compile_member_access(vm, node->left, true);
    } else if (node->right->type == AST_OBJECT) {
        compile_object_literal(vm, node->right);
        compile_variable(vm, node->left, true);
    } else {
        compile_expression(vm, node->right, true); 
        compile_variable(vm, node->left, true);
    }
}

/**
 * Compiles an AST node representing a unary expression to bytecode.
 *
 * This function handles unary negation, logical negation, prefix and postfix
 * increment and decrement, and array access. It compiles the subexpressions
 * of the node, and emits the appropriate bytecode to perform the unary
 * operation. If the result of the expression is not used, it emits an OP_POP
 * instruction to pop the result off the stack.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the unary expression to be compiled.
 * @param result_used A boolean indicating whether the result of the expression
 *                    is used by another expression.
 */
static void compile_unary_expr(ApexVM *vm, AST *node, bool result_used) {
    bool is_postfix;
    
    if (node->value.strval == apexStr_new("!", 1)) {
        compile_expression(vm, node->right, true);
        EMIT_OP(vm, OP_NOT);
        return;
    } 
    if (node->value.strval == apexStr_new("-", 1)) {
        compile_expression(vm, node->right, true);
        EMIT_OP(vm, OP_NEGATE);
        return;
    } 
    if (node->value.strval == apexStr_new("+", 1)) {
        compile_expression(vm, node->right, true);
        EMIT_OP(vm, OP_POSITIVE);
        return;
    } 
    is_postfix = (node->right == NULL);
    if ((is_postfix && node->left->type == AST_ARRAY_ACCESS) || 
        (!is_postfix && node->right->type == AST_ARRAY_ACCESS)) {
        UPDATE_SRCLOC(vm, node);
        AST *array_node = is_postfix ? node->left : node->right;
        compile_expression(vm, array_node->left, true); // array
        compile_expression(vm, array_node->right, true); // index
    }
    if (node->value.strval == apexStr_new("++", 2)) {
        if (vm->in_function) {          
            if (is_postfix) {
                EMIT_OP_STR(vm, OP_POST_INC_LOCAL, node->left->value.strval);
            } else {
                EMIT_OP_STR(vm, OP_PRE_INC_LOCAL, node->right->value.strval);
            }       
        } else {     
            if (is_postfix) {
                EMIT_OP_STR(vm, OP_POST_INC_GLOBAL, node->left->value.strval);
            } else {
                EMIT_OP_STR(vm, OP_PRE_INC_GLOBAL, node->right->value.strval);
            }
        }
    } else if (node->value.strval == apexStr_new("--", 2)) {
        if (vm->in_function) {   
            if (is_postfix) {
                EMIT_OP_STR(vm, OP_POST_DEC_LOCAL, node->left->value.strval);
            } else {
                EMIT_OP_STR(vm, OP_PRE_DEC_LOCAL, node->right->value.strval);
            }        
        } else {
            if (is_postfix) {
                EMIT_OP_STR(vm, OP_POST_DEC_GLOBAL, node->left->value.strval);
            } else {
                EMIT_OP_STR(vm, OP_PRE_DEC_GLOBAL, node->right->value.strval);
            }       
        }
    }
    if (!result_used) {
        EMIT_OP(vm, OP_POP);
    }    
}

/**
 * Compiles an AST node representing the creation of a new object to bytecode.
 *
 * This function compiles the class/object name and any arguments for the
 * constructor, and emits the OP_NEW instruction with the number of arguments.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the new expression to be compiled.
 */
static void compile_new(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    int argc = 0;

    compile_argument_list(vm, node->right, &argc);

    // Compile the class/object name(e.g., 'obj' in 'obj.new()')
    compile_expression(vm, node->left, true);
    EMIT_OP_INT(vm, OP_NEW, argc);
}

/**
 * Compiles an AST node representing member access to bytecode.
 *
 * This function compiles the base object and the member name, emitting the
 * appropriate bytecode instruction to either get or set a member, depending
 * on the value of is_assignment.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the member access to be compiled.
 * @param is_assignment A boolean indicating whether the access is an
 *                      assignment. If true, an instruction to set the member
 *                      is emitted. If false, an instruction to get the member
 *                      is emitted.
 */
static void compile_member_access(ApexVM *vm, AST *node, bool is_assignment) {
    UPDATE_SRCLOC(vm, node);

    // Compile the base object(e.g., 'obj' in 'obj.member')
    compile_expression(vm, node->left, true); 

    ApexString *name = node->right->value.strval;
    if (is_assignment) {
        EMIT_OP_STR(vm, OP_SET_MEMBER, name);
    } else {
        EMIT_OP_STR(vm, OP_GET_MEMBER, name);
    }
}

/**
 * Compiles an AST node representing a member function call to bytecode.
 *
 * This function compiles the base object, the function name, and the arguments
 * to the function, and emits an instruction to call the member function with
 * the correct arguments and return value.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the member function call to be
 *             compiled.
 */
static void compile_member_function_call(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    int argc = 0;

    // Compile the base object (e.g., 'obj' in 'obj.method()')
    compile_expression(vm, node->left, true);

    ApexString *name = node->right->value.strval;
    compile_argument_list(vm, node->value.ast_node, &argc);
    EMIT_OP_STR(vm, OP_GET_MEMBER, name);
    EMIT_OP_INT(vm, OP_CALL_MEMBER, argc);
}

/**
 * Compiles an AST node representing an object literal to bytecode.
 *
 * This function iterates through the members of the object, compiling each
 * key-value pair and emitting an instruction to create an object with the
 * correct key-value pairs. The instruction is emitted with the count of
 * key-value pairs as the argument.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the object literal to be compiled.
 */
static void compile_object_literal(ApexVM *vm, AST *node) {
    int n = 0;
    UPDATE_SRCLOC(vm, node);

    ApexString *name = node->value.strval;
    AST *current = node->right;
    while (current) {
        if (current->type == AST_KEY_VALUE_PAIR) {
            compile_expression(vm, current->left, true); // Key
            compile_expression(vm, current->right, true); // Value
            n++;
        }
        current = current->right;
    }

    ApexObject *obj = apexVal_newobject();
    apexSym_setglobal(&vm->global_table, name->value, apexVal_maketype(obj));
    
    EMIT_OP_STR(vm, OP_PUSH_STR, name);
    EMIT_OP_INT(vm, OP_CREATE_OBJECT, n);
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
static void compile_expression(ApexVM *vm, AST *node, bool result_used) {
    UPDATE_SRCLOC(vm, node);
    switch (node->type) {
    case AST_INT:
        EMIT_OP_INT(vm, OP_PUSH_INT, atoi(node->value.strval->value));
        break;

    case AST_DBL:
        double dbl = strtod(node->value.strval->value, NULL);
        EMIT_OP_DBL(vm, OP_PUSH_DBL, dbl);
        break;

    case AST_STR:
        EMIT_OP_STR(vm, OP_PUSH_STR, node->value.strval);
        break;

    case AST_NULL:
        EMIT_OP(vm, OP_PUSH_NULL);
        break;

    case AST_BOOL:
        EMIT_OP_BOOL(vm, OP_PUSH_BOOL, node->value.strval == apexStr_new("true", 4) ? true : false);
        break;
    
    case AST_BINARY_EXPR:
        compile_expression(vm, node->left, true);
        compile_expression(vm, node->right, true);
        if (node->value.strval == apexStr_new("+", 1)) {
            EMIT_OP(vm, OP_ADD);
        } else if (node->value.strval == apexStr_new("-", 1)) {
            EMIT_OP(vm, OP_SUB);
        } else if (node->value.strval == apexStr_new("*", 1)) {
            EMIT_OP(vm, OP_MUL);
        } else if (node->value.strval == apexStr_new("/", 1)) {
            EMIT_OP(vm, OP_DIV);
        } else if (node->value.strval == apexStr_new("==", 2)) {
            EMIT_OP(vm, OP_EQ);
        } else if (node->value.strval == apexStr_new("!=", 2)) {
            EMIT_OP(vm, OP_NE);
        } else if (node->value.strval == apexStr_new("<", 1)) {
            EMIT_OP(vm, OP_LT);
        } else if (node->value.strval == apexStr_new("<=", 2)) {
            EMIT_OP(vm, OP_LE);
        } else if (node->value.strval == apexStr_new(">", 1)) {
            EMIT_OP(vm, OP_GT);
        } else if (node->value.strval == apexStr_new(">=", 2)) {
            EMIT_OP(vm, OP_GE);
        }
        break;

    case AST_UNARY_EXPR:
        compile_unary_expr(vm, node, result_used);
        break;

    case AST_LOGICAL_EXPR: {
        int short_circuit_jmp;
        int end_jmp;
        compile_expression(vm, node->left, true);

        if (node->value.strval == apexStr_new("&&", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_n - 1;
            compile_expression(vm, node->right, true);
            EMIT_OP(vm, OP_JUMP);
            end_jmp = vm->chunk->ins_n - 1;
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_n - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, false);
            vm->chunk->ins[end_jmp].value.intval = vm->chunk->ins_n - end_jmp - 1;
        } else if (node->value.strval == apexStr_new("||", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_n - 1;
            compile_expression(vm, node->right, true);
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_n - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, true);
        }
        break;
    }

    case AST_VAR:
        compile_variable(vm, node, false);        
        break;

    case AST_ARRAY:
        compile_array(vm, node);
        break;

    case AST_OBJECT:
        compile_object_literal(vm, node);
        break;

    case AST_ARRAY_ACCESS:
        compile_array_access(vm, node, false);
        break;

    case AST_ASSIGNMENT:
        compile_assignment(vm, node);
        break;

    case AST_FN_CALL: 
        if (node->left->type == AST_MEMBER_ACCESS) {
            compile_member_function_call(vm, node->left);
        } else {
            compile_function_call(vm, node);
        }
        break;

    case AST_LIB_CALL:
        compile_library_call(vm, node);
        break;

    case AST_NEW:
        compile_new(vm, node);
        break;

    case AST_MEMBER_ACCESS:
        compile_member_access(vm, node, false);
        break;

    default:
        apexErr_fatal(node->srcloc, "Unhandled AST node type: %d", node->type);
    }
}

/**
 * Compiles an AST node representing an include statement to bytecode.
 *
 * This function opens the specified file and reads it into memory, then
 * lexes and parses the source code in the file. If the parse is successful,
 * the ASTs are compiled recursively. If the parse fails, a warning is emitted
 * with the specified source location.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the include statement to be compiled.
 */
static void compile_include(ApexVM *vm, AST *node) {
    const char *incpath = node->value.strval->value;
    const char *filepath = node->srcloc.filename;
    FILE *file;
    Lexer lexer;
    Parser parser;
    AST *program;
    
    char *lslash = strrchr(filepath, '/');
#ifdef _WIN32
    if (!lslash) {
        lslash = strrchr(filepath, '\\');
    }
#endif
    
    if (lslash && filepath[0] != '/') {
        size_t dir_len = lslash - filepath + 1;
        size_t fullpath_len = dir_len + strlen(incpath) + 1;
        char *fullpath = apexMem_alloc(fullpath_len);
        snprintf(fullpath, fullpath_len, "%.*s%s", (int)dir_len, filepath, incpath);
        
        file = fopen(fullpath, "r");
        if (!file) {
            apexErr_warn(node->srcloc,"cannot include specified path: %s", fullpath);
        }
        free(fullpath);
    } else {
        file = fopen(incpath, "r");
        if (!file) {
            apexErr_warn(node->srcloc, "cannot include specified path: %s", incpath);
        }
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *source = apexMem_alloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    init_lexer(&lexer, filepath, source);
    init_parser(&parser, &lexer);

    program = parse_program(&parser);
    if (program) {
        if (program->left) {
            compile_statement(vm, program->left);
        }
        
        if (program->right) {
            compile_statement(vm, program->right);
        }  
    }

    free_ast(program);
    free_parser(&parser);
    free(source);
}

static void compile_switch(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    AST *left = node->left;
    
    int end_jumps[256];
    int end_jumps_n = 0;

    for (AST *case_node = node->right; case_node; case_node = case_node->right) {
        if (case_node->type == AST_CASE) {
            compile_expression(vm, left, true);
            // Compile the case value (e.g., 'one', 1.5, true)
            compile_expression(vm, case_node->left, true);

            // Emit a comparison (switch_value == case_value)
            EMIT_OP(vm, OP_EQ);

            // Emit a jump to skip this case body
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            int skip_jump = vm->chunk->ins_n - 1;
            
            // Compile the case body
            compile_statement(vm, case_node->right);
            EMIT_OP(vm, OP_JUMP);
            end_jumps[end_jumps_n++] = vm->chunk->ins_n - 1;
            

            // Patch the jump to this case
            vm->chunk->ins[skip_jump].value.intval = vm->chunk->ins_n - skip_jump - 1;
        }
    }

    // Compile the default case if present
    if (node->value.ast_node) {
        compile_statement(vm, node->value.ast_node);
    }
    
    for (int i = 0; i < end_jumps_n; i++) {
        vm->chunk->ins[end_jumps[i]].value.intval = vm->chunk->ins_n - end_jumps[i] - 1;
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
        compile_expression(vm, condition, true);
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

    switch (node->type) {
    case AST_IF: {
        int false_jmp_i;
        int true_jmp_i;

        UPDATE_SRCLOC(vm, node);

        compile_expression(vm, node->left, true); // Condition
        EMIT_OP(vm, OP_JUMP_IF_FALSE);
        false_jmp_i = vm->chunk->ins_n - 1;

        compile_statement(vm, node->right); // Block or statement 
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
    case AST_SWITCH:
        compile_switch(vm, node);
        break;

    case AST_WHILE: 
        compile_loop(vm, node->left, node->right, NULL);
        break;

    case AST_FOR: 
        compile_statement(vm, node->left);
        compile_loop(vm, node->right, node->value.ast_node->right, node->value.ast_node->left);
        break;

    case AST_INCLUDE:
        compile_include(vm, node);
        break;
        
    case AST_CONTINUE:
        if (vm->loop_start == -1) {
            apexErr_fatal(node->srcloc, "invalid 'continue' outside of loop on line %d");
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_start - vm->chunk->ins_n - 1);
        break;

    case AST_BREAK:
        if (vm->loop_end == -1) {
            apexErr_fatal(node->srcloc,"invalid 'break' outside of loop on line %d");
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_end - vm->chunk->ins_n - 1);
        break;

    case AST_FN_DECL:
        compile_function_declaration(vm, node);
        break;

    case AST_RETURN:
        compile_expression(vm, node->left, true);
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
        if (node->right && node->right->type != AST_CASE) {
            compile_statement(vm, node->right);
        }
        break;

    default:
        compile_expression(vm, node, false);
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
    if (program->left) {
        compile_statement(vm, program->left);
    }
    
    if (program->right) {
        compile_statement(vm, program->right);
    }    
    EMIT_OP(vm, OP_HALT);
}
