#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "apexCode.h"
#include "apexVM.h"
#include "apexVal.h"
#include "apexErr.h"
#include "apexSym.h"
#include "apexMem.h"
#include "apexStr.h"
#include "apexLib.h"
#include "apexLex.h"
#include "apexParse.h"
#include "apexUtil.h"

#define EMIT_OP_INT(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makeint(value))
#define EMIT_OP_DBL(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makedbl(value))
#define EMIT_OP_STR(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makestr(value))
#define EMIT_OP_BOOL(vm, opcode, value) emit_instruction(vm, opcode, apexVal_makebool(value))
#define EMIT_OP(vm, opcode) emit_instruction(vm, opcode, zero_value())
#define UPDATE_SRCLOC(vm, node) vm->srcloc = node->srcloc

static bool compile_expression(ApexVM *vm, AST *node, bool use_result);
static bool compile_statement(ApexVM *vm, AST *node);
static bool compile_member_access(ApexVM *vm, AST *node, bool is_assignment);
static bool compile_object_literal(ApexVM *vm, AST *node);

/**
 * Ensures that the instruction buffer has enough capacity to store one more
 * instruction.
 */
static void ensure_capacity(ApexVM *vm) {
    if (vm->chunk->ins_count + 1 >= vm->chunk->ins_size) {
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
    vm->chunk->ins[vm->chunk->ins_count].srcloc = vm->srcloc;
    vm->chunk->ins[vm->chunk->ins_count].value = value;
    vm->chunk->ins[vm->chunk->ins_count].opcode = opcode;
    vm->chunk->ins_count++;
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
 * Compiles a parameter list from an AST node to an array of strings.
 *
 * This function takes an AST node representing a parameter list and
 * compiles it to an array of strings containing the parameter names.
 *
 * @param param_list The AST node representing the parameter list to be
 *                   compiled.
 * @param argc A pointer to an integer which will be set to the number
 *                of parameters in the list.
 * @return An array of strings containing the parameter names, or NULL if
 *         the parameter list is empty.
 */
static char **compile_parameter_list(AST *param_list, int *argc, bool *have_variadic) {
    int pcount = 0;
    int psize = 8;
    char **params = apexMem_alloc(sizeof(char *) * psize);

    while (param_list) {
        if (param_list->type == AST_BLOCK) {
            break;
        } 
        
        if (param_list->type == AST_VAR || param_list->type == AST_VARIADIC) {
            char *param_name = param_list->value.strval->value;
            if (pcount >= psize) {
                psize *= 2;
                params = apexMem_realloc(params, sizeof(char *) * psize);
            }
            if (param_list->type == AST_VARIADIC) {
                *have_variadic = true;
            }
            params[pcount++] = param_name;
            break;
        } else if (param_list->type == AST_PARAMETER_LIST) {
            AST *param = param_list->left;
            char *param_name = param->value.strval->value;
            if (param->type != AST_VAR && param->type != AST_VARIADIC) {
                apexErr_syntax(param->srcloc, "expected parameter to be a variable");
                return NULL;
            }
            if (pcount >= psize) {
                psize *= 2;
                params = apexMem_realloc(params, sizeof(char *) * psize);
            }
            if (param->type == AST_VARIADIC) {
                *have_variadic = true;
            }
            params[pcount++] = param_name;

            if (param_list->right && 
               (param_list->right->type == AST_PARAMETER_LIST ||
                param_list->right->type == AST_VAR ||
                param_list->right->type == AST_VARIADIC)) {
                param_list = param_list->right;
            } else {
                break;
            }
        } else {
            apexErr_syntax(param_list->srcloc,"Invalid AST node in parameter list");
            return NULL;
        }
    }
    *argc = pcount;
    return params;
}


/**
 * Compiles an AST node representing an array literal to bytecode.
 *
 * This function iterates through the elements of the array, compiling each
 * element or key-value pair and emitting the appropriate instructions to
 * construct an array in the virtual machine. It handles both regular array
 * elements and associative arrays with key-value pairs. The instruction is
 * emitted with the count of elements or pairs as the argument.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the array literal to be compiled.
 * @return true if the array was compiled successfully, false otherwise.
 */
static bool compile_array(ApexVM *vm, AST *node) {
    int count = 0;
    UPDATE_SRCLOC(vm, node);

    AST *current = node->right;
    while (current) {
        if (current->type == AST_KEY_VALUE_PAIR) {
            if (!compile_expression(vm, current->left, true) || // key
                !compile_expression(vm, current->right, true)) { // value
                return false;
            }
            count++;
        } else if (!current->right || current->right->type != AST_KEY_VALUE_PAIR) {
            EMIT_OP_INT(vm, OP_PUSH_INT, count); 
            if (!compile_expression(vm, current, true)) { // array element
                return false;
            }
            count++;
        }
        
        current = current->right->right;
    }

    EMIT_OP_INT(vm, OP_CREATE_ARRAY, count);
    return true;
}


/**
 * Compiles an AST node representing an array access to bytecode.
 *
 * This function compiles the array expression and the index expression, and
 * emits an instruction to either set or get an element from the array,
 * depending on the value of is_assignment.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the array access to be compiled.
 * @param is_assignment A boolean indicating whether the access is an
 *                      assignment. If true, an instruction to set the element
 *                      is emitted. If false, an instruction to get the element
 *                      is emitted.
 * @return A boolean indicating whether the compilation was successful.
 */
static bool compile_array_access(ApexVM *vm, AST *node, bool is_assignment) {
    UPDATE_SRCLOC(vm, node);

    if (!compile_expression(vm, node->left, true) || // array
        !compile_expression(vm, node->right, true)) { // index
        return false;
    }

    if (is_assignment) {
        EMIT_OP(vm, OP_SET_ELEMENT);
    } else {
        EMIT_OP(vm, OP_GET_ELEMENT);
    }
    return true;
}

/**
 * Recursively compiles an argument list to bytecode.
 *
 * This function iterates through the argument list, compiling each
 * argument and emitting an instruction to push it onto the stack.
 * If the argc parameter is not NULL, it is incremented for each
 * argument compiled.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param argument_list The AST node representing the argument list to be
 *                      compiled.
 * @param argc A pointer to an integer that will be set to the number of
 *             arguments in the argument list.
 * @return true if the argument list was compiled successfully, false
 *         otherwise.
 */
static bool compile_argument_list(ApexVM *vm, AST *argument_list, int *argc) {
    if (argument_list == NULL) {
        return true;
    }
    
    if (!compile_argument_list(vm, argument_list->left, argc) ||
        !compile_expression(vm, argument_list->right, true)) {
        return false;
    }
    if (argc) {
        (*argc)++;
    }
    return true;
}

/**
 * Compiles an AST node representing a function declaration to bytecode.
 *
 * This function handles both member functions and standalone functions.
 * It sets up the function environment, including starting the function
 * scope, compiling the parameter list, and verifying the object's existence
 * for member functions. The function body is then compiled, and bytecode
 * instructions are emitted for function start, return, and end.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and symbol tables.
 * @param node The AST node representing the function declaration to be
 *             compiled.
 * @return true if the function declaration was compiled successfully,
 *         false otherwise.
 */
static bool compile_function_declaration(ApexVM *vm, AST *node) {
    if (node->left->type == AST_MEMBER_FN) {        
        const char *objname = node->left->left->value.strval->value;
        const char *fnname = node->left->right->value.strval->value;
        
        vm->in_function = true;
        EMIT_OP(vm, OP_FUNCTION_START);
        push_scope(&vm->local_scopes);
        int argc = 0;
        bool have_variadic;
        char **params = compile_parameter_list(node->value.ast_node, &argc, &have_variadic);
        if (!params) {
            return false;
        }
        ApexFn *fn = apexVal_newfn(fnname, params, argc, have_variadic, vm->chunk->ins_count);
        ApexValue value;
        if (!apexSym_getglobal(&value, &vm->global_table, objname)) {
            apexErr_syntax(node->srcloc, "object %s not found", objname);
            return false;
        }
        if (value.type != APEX_VAL_TYPE) {
            apexErr_syntax(node->srcloc, "%s is not an object", objname);
            return false;
        }
        ApexObject *obj = value.objval;
        apexVal_objectset(obj, fnname, apexVal_makefn(fn));
        if (!compile_statement(vm, node->right)) { // function body
            return false;
        }
        EMIT_OP(vm, OP_RETURN);
        pop_scope(&vm->local_scopes);
        EMIT_OP(vm, OP_FUNCTION_END);
        vm->in_function = false;
    } else {
        const char *fnname = node->left->value.strval->value;
        int argc = 0;
        bool have_variadic;
        vm->in_function = true;
        EMIT_OP(vm, OP_FUNCTION_START);
        push_scope(&vm->local_scopes);
        char **params = compile_parameter_list(node->value.ast_node, &argc, &have_variadic);
        if (!params) {
            return false;
        }
        ApexFn *fn = apexVal_newfn(fnname, params, argc, have_variadic, vm->chunk->ins_count);
        apexSym_setglobal(&vm->global_table, fnname, apexVal_makefn(fn));
        if (!compile_statement(vm, node->right)) { // function body
            return false;
        }
        EMIT_OP(vm, OP_RETURN);
        pop_scope(&vm->local_scopes);
        EMIT_OP(vm, OP_FUNCTION_END);
        vm->in_function = false;
    }
    return true;
}


/**
 * Compiles an AST node representing a function call to bytecode.
 *
 * This function compiles the argument list and the function name, and
 * emits an instruction to call the function with the correct number of
 * arguments.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the function call to be
 *             compiled.
 * @return true if the function call was compiled successfully, false
 *         otherwise.
 */
static bool compile_function_call(ApexVM *vm, AST *node) {
    ApexString *fn_name = node->left->value.strval;
    int argc = 0;
    UPDATE_SRCLOC(vm, node);
    if (!compile_argument_list(vm, node->right, &argc)) {
        return false;
    }
    EMIT_OP_STR(vm, OP_GET_GLOBAL, fn_name);
    EMIT_OP_INT(vm, OP_CALL, argc);
    return true;
}

/**
 * Compiles an AST node representing a library function call to bytecode.
 *
 * This function compiles the argument list, library name, and function name, and
 * emits an instruction to call the library function with the correct number of
 * arguments.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the library function call to be
 *             compiled.
 * @return true if the library function call was compiled successfully, false
 *         otherwise.
 */
static bool compile_library_call(ApexVM *vm, AST *node) {
    int argc = 0;
    if (!compile_argument_list(vm, node->value.ast_node, &argc)) {
        return false;
    }

    EMIT_OP_STR(vm, OP_PUSH_STR, node->left->value.strval); // Library name
    EMIT_OP_STR(vm, OP_PUSH_STR, node->right->value.strval); // Function name
    EMIT_OP_INT(vm, OP_CALL_LIB, argc);
    return true;
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
static bool compile_assignment(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);

    if (node->type == AST_ASSIGN_ADD ||
        node->type == AST_ASSIGN_SUB ||
        node->type == AST_ASSIGN_MUL ||
        node->type == AST_ASSIGN_DIV ||
        node->type == AST_ASSIGN_MOD) {
        if (node->left->type == AST_ARRAY_ACCESS) {
            if (!compile_array_access(vm, node->left, false)) {
                return false;
            }
        } else if (node->left->type == AST_MEMBER_ACCESS) {
            if (!compile_member_access(vm, node->left, false)) {
                return false;
            }
        } else {
            compile_variable(vm, node->left, false);            
        }
        if (!compile_expression(vm, node->right, true)) {
            return false;
        }
        switch (node->type) {
        case AST_ASSIGN_ADD:
            EMIT_OP(vm, OP_ADD);
            break;
        case AST_ASSIGN_SUB:
            EMIT_OP(vm, OP_SUB);
            break;
        case AST_ASSIGN_MUL:
            EMIT_OP(vm, OP_MUL);
            break;
        case AST_ASSIGN_DIV:
            EMIT_OP(vm, OP_DIV);
            break;
        case AST_ASSIGN_MOD:
            EMIT_OP(vm, OP_MOD);
            break;
        default:
            break;
        }
        compile_variable(vm, node->left, true);
        return true;
    }

    if (node->left->type == AST_ARRAY_ACCESS) { 
        return compile_expression(vm, node->right, true) &&      
            compile_array_access(vm, node->left, true);
    } else if (node->right->type == AST_ARRAY) {
        if (!compile_array(vm, node->right)) {
            return false;
        }
        compile_variable(vm, node->left, true);
    } else if (node->left->type == AST_MEMBER_ACCESS) {
        return compile_expression(vm, node->right, true) &&    
            compile_member_access(vm, node->left, true);
    } else if (node->right->type == AST_OBJECT) {
        return compile_object_literal(vm, node->right);
    } else {
        if (!compile_expression(vm, node->right, true)) {
            return false;
        }
        compile_variable(vm, node->left, true);
    }
    return true;
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
static bool compile_unary_expr(ApexVM *vm, AST *node, bool result_used) {
    bool is_postfix;
    
    if (node->type == AST_UNARY_NOT) {
        if (!compile_expression(vm, node->right, true)) {
            return false;
        }
        EMIT_OP(vm, OP_NOT);
        return true;
    } 
    if (node->type == AST_UNARY_SUB) {
        if (!compile_expression(vm, node->right, true)) {
            return false;
        }
        EMIT_OP(vm, OP_NEGATE);
        return true;
    } 
    if (node->type == AST_UNARY_ADD) {
        if (!compile_expression(vm, node->right, true)) {
            return false;
        }
        EMIT_OP(vm, OP_POSITIVE);
        return true;
    } 
    is_postfix = (node->right == NULL);
    if ((is_postfix && node->left->type == AST_ARRAY_ACCESS) || 
        (!is_postfix && node->right->type == AST_ARRAY_ACCESS)) {
        UPDATE_SRCLOC(vm, node);
        AST *array_node = is_postfix ? node->left : node->right;
        if (!compile_expression(vm, array_node->left, true) || // array
            !compile_expression(vm, array_node->right, true)) { // index
            return false;
        }
    }
    if (node->type == AST_UNARY_INC) {
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
    } else if (node->type == AST_UNARY_DEC) {
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
    return true;
}

/**
 * Compiles an AST node representing a binary expression to bytecode.
 *
 * This function compiles the left and right subexpressions of the node, and
 * emits an instruction to perform the binary operation specified by the
 * node's type.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the binary expression to be
 *             compiled.
 * @return true if the binary expression was compiled successfully, false
 *         otherwise.
 */
static bool compile_binary_expr(ApexVM *vm, AST *node) {
    if (!compile_expression(vm, node->left, true) || 
        !compile_expression(vm, node->right, true)) {
        return false;
    }        
    switch (node->type) {
    case AST_BIN_ADD:
        EMIT_OP(vm, OP_ADD);
        break;
    case AST_BIN_SUB: 
        EMIT_OP(vm, OP_SUB);
        break;
    case AST_BIN_MUL:
        EMIT_OP(vm, OP_MUL);
        break;
    case AST_BIN_DIV:
        EMIT_OP(vm, OP_DIV);
        break;
    case AST_BIN_MOD:
        EMIT_OP(vm, OP_MOD);
        break;
    case AST_BIN_EQ:
        EMIT_OP(vm, OP_EQ);
        break;
    case AST_BIN_NE:
        EMIT_OP(vm, OP_NE);
        break;
    case AST_BIN_LT:
        EMIT_OP(vm, OP_LT);
        break;
    case AST_BIN_LE:
        EMIT_OP(vm, OP_LE);
        break;
    case AST_BIN_GT:
        EMIT_OP(vm, OP_GT);
        break;
    case AST_BIN_GE:
        EMIT_OP(vm, OP_GE);
        break;
    default:
        break;
    }
    return true;
}

/**
 * Compiles an AST node representing the `new` operator to bytecode.
 *
 * This function compiles the argument list and the class/object name, and
 * emits an instruction to create a new object with the correct number of
 * arguments.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the `new` operator to be compiled.
 * @return true if the `new` operator was compiled successfully, false
 *         otherwise.
 */
static bool compile_new(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    int argc = 0;

    if (!compile_argument_list(vm, node->right, &argc)) {
        return false;
    }

    // Compile the class/object name(e.g., 'obj' in 'obj.new()')
    if (!compile_expression(vm, node->left, true)) {
        return false;
    }
    EMIT_OP_INT(vm, OP_NEW, argc);
    return true;
}

/**
 * Compiles an AST node representing a member access to bytecode.
 *
 * This function compiles the base object and the member name, and emits
 * an instruction to either set or get the member value, depending on the
 * value of is_assignment.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the member access to be compiled.
 * @param is_assignment A boolean indicating whether the access is an
 *                      assignment. If true, an instruction to set the member
 *                      is emitted. If false, an instruction to get the member
 *                      is emitted.
 * @return true if the member access was compiled successfully, false
 *         otherwise.
 */
static bool compile_member_access(ApexVM *vm, AST *node, bool is_assignment) {
    UPDATE_SRCLOC(vm, node);

    // Compile the base object(e.g., 'obj' in 'obj.member')
    if (!compile_expression(vm, node->left, true)) {
        return false;
    }

    ApexString *name = node->right->value.strval;
    if (is_assignment) {
        EMIT_OP_STR(vm, OP_SET_MEMBER, name);
    } else {
        EMIT_OP_STR(vm, OP_GET_MEMBER, name);       
    }
    return true;
}

/**
 * Compiles an AST node representing a member function call to bytecode.
 *
 * This function compiles the argument list, base object, and member function
 * name, and emits an instruction to call the member function with the correct
 * number of arguments.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the member function call to be
 *             compiled.
 * @return true if the member function call was compiled successfully, false
 *         otherwise.
 */
static bool compile_member_function_call(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    int argc = 0;

    // Compile the base object (e.g., 'obj' in 'obj.method()')
    if (!compile_argument_list(vm, node->right, &argc) ||
        !compile_expression(vm, node->left->left, true)) {
        return false;
    }

    ApexString *name = node->left->right->value.strval;
    
    EMIT_OP_INT(vm, OP_PUSH_INT, argc);
    EMIT_OP_STR(vm, OP_CALL_MEMBER, name);
    return true;
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
static bool compile_object_literal(ApexVM *vm, AST *node) {
    int count = 0;
    UPDATE_SRCLOC(vm, node);

    ApexString *name = node->value.strval;
    AST *current = node->right;
    while (current) {
        if (current->type == AST_KEY_VALUE_PAIR) {
            if (!compile_expression(vm, current->left, true) || // Key
                !compile_expression(vm, current->right, true)) { // Value
                return false;
            }
            count++;
        }
        current = current->right;
    }

    ApexObject *obj = apexVal_newobject(name->value);
    apexSym_setglobal(&vm->global_table, name->value, apexVal_maketype(obj));
    
    EMIT_OP_STR(vm, OP_PUSH_STR, name);
    EMIT_OP_INT(vm, OP_CREATE_OBJECT, count);
    return true;
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
static bool compile_expression(ApexVM *vm, AST *node, bool result_used) {
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
        EMIT_OP_BOOL(vm, OP_PUSH_BOOL, 
            node->value.strval == apexStr_new("true", 4) ? true : false);
        break;
    
    case AST_BIN_ADD:
    case AST_BIN_SUB:
    case AST_BIN_MUL:
    case AST_BIN_DIV:
    case AST_BIN_MOD:
    case AST_BIN_EQ:
    case AST_BIN_NE:
    case AST_BIN_LT:
    case AST_BIN_LE:
    case AST_BIN_GT:
    case AST_BIN_GE:
        return compile_binary_expr(vm, node);

    case AST_UNARY_ADD:
    case AST_UNARY_SUB:
    case AST_UNARY_NOT:
    case AST_UNARY_INC:
    case AST_UNARY_DEC:
        return compile_unary_expr(vm, node, result_used);

    case AST_LOGICAL_EXPR: {
        int short_circuit_jmp;
        int end_jmp;
        if (!compile_expression(vm, node->left, true)) {
            return false;
        }

        if (node->value.strval == apexStr_new("&&", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_count - 1;
            if (!compile_expression(vm, node->right, true)) {
                return false;
            }
            EMIT_OP(vm, OP_JUMP);
            end_jmp = vm->chunk->ins_count - 1;
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_count - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, false);
            vm->chunk->ins[end_jmp].value.intval = vm->chunk->ins_count - end_jmp - 1;
        } else if (node->value.strval == apexStr_new("||", 2)) {
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            short_circuit_jmp = vm->chunk->ins_count - 1;
            if (!compile_expression(vm, node->right, true)) {
                return false;
            }
            vm->chunk->ins[short_circuit_jmp].value.intval = vm->chunk->ins_count - short_circuit_jmp - 1;
            EMIT_OP_BOOL(vm, OP_PUSH_BOOL, true);
        }
        break;
    }

    case AST_VAR:
        compile_variable(vm, node, false);        
        break;

    case AST_ARRAY:
        return compile_array(vm, node);

    case AST_OBJECT:
        return compile_object_literal(vm, node);

    case AST_ARRAY_ACCESS:
        return compile_array_access(vm, node, false);

    case AST_ASSIGNMENT:
    case AST_ASSIGN_ADD:
    case AST_ASSIGN_SUB:
    case AST_ASSIGN_MUL:
    case AST_ASSIGN_DIV:
    case AST_ASSIGN_MOD:        
        return compile_assignment(vm, node);

    case AST_FN_CALL: 
        if (node->left->type == AST_MEMBER_ACCESS) {
            if (!compile_member_function_call(vm, node)) {
                return false;
            }
        } else {
            if (!compile_function_call(vm, node)) {
                return false;
            }
        }
        break;

    case AST_LIB_CALL:
        return compile_library_call(vm, node);

    case AST_NEW:
        return compile_new(vm, node);

    case AST_MEMBER_ACCESS:
        return compile_member_access(vm, node, false);

    default:
        apexErr_syntax(node->srcloc, "Unhandled AST node type: %d", node->type);
        return false;
    }
    return true;
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
static bool compile_include(ApexVM *vm, AST *node) {
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
            apexErr_syntax(node->srcloc, "cannot include specified path: %s", fullpath);
            return false;
        }
        free(fullpath);
    } else {
        file = fopen(incpath, "r");
        if (!file) {
            apexErr_syntax(node->srcloc, "cannot include specified path: %s", incpath);
            return false;
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
            if (!compile_statement(vm, program->left)) {
                return false;
            }
        }
        
        if (program->right) {
            if (!compile_statement(vm, program->right)) {
                return false;
            }
        }  
    }

    free_ast(program);
    free_parser(&parser);
    free(source);
    return true;
}

/**
 * Compiles an AST node representing a switch statement to bytecode.
 *
 * This function compiles the switch condition and each case condition, emits
 * a comparison instruction, and then emits a jump instruction to skip the case
 * body if the comparison is false. The case body is then compiled, and an
 * instruction is emitted to jump to the end of the switch statement after the
 * case body is finished. If a default case is present, it is compiled after all
 * other cases.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @param node The AST node representing the switch statement to be compiled.
 * @return true if the switch statement was compiled successfully, false
 *         otherwise.
 */
static bool compile_switch(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    AST *left = node->left;
    
    int end_jumps[256];
    int end_jumps_n = 0;

    for (AST *case_node = node->right; case_node; case_node = case_node->right) {
        if (case_node->type == AST_CASE) {
            if (!compile_expression(vm, left, true) ||
                !compile_expression(vm, case_node->left, true)) {
                return false;
            }
            // Emit a comparison (switch_value == case_value)
            EMIT_OP(vm, OP_EQ);

            // Emit a jump to skip this case body
            EMIT_OP(vm, OP_JUMP_IF_FALSE);
            int skip_jump = vm->chunk->ins_count - 1;
            
            // Compile the case body
            if (!compile_statement(vm, case_node->right)) {
                return false;
            }
            EMIT_OP(vm, OP_JUMP);
            end_jumps[end_jumps_n++] = vm->chunk->ins_count - 1;
            

            // Patch the jump to this case
            vm->chunk->ins[skip_jump].value.intval = vm->chunk->ins_count - skip_jump - 1;
        }
    }

    // Compile the default case if present
    if (node->value.ast_node) {
        if (!compile_statement(vm, node->value.ast_node)) {
            return false;
        }
    }
    
    for (int i = 0; i < end_jumps_n; i++) {
        vm->chunk->ins[end_jumps[i]].value.intval = vm->chunk->ins_count - end_jumps[i] - 1;
    }
    return true;
}

/**
 * Compiles an AST node representing a loop to bytecode.
 *
 * This function handles the compilation of loop constructs such as while
 * loops and for loops. It sets up the loop environment, compiles the
 * loop condition, body, and any increment expression. If the loop condition
 * is present, it emits a conditional jump instruction to exit the loop
 * when the condition evaluates to false. It ensures that the loop body is
 * entered initially and that the increment expression is executed after
 * each iteration. It restores the previous loop state after completion.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk and loop state.
 * @param condition The AST node representing the loop condition expression.
 * @param body The AST node representing the loop body to be compiled.
 * @param increment The AST node representing the increment expression,
 *                  executed after each iteration.
 * @return true if the loop was compiled successfully, false otherwise.
 */
static bool compile_loop(ApexVM *vm, AST *condition, AST *body, AST *increment) {
    int previous_loop_start = vm->loop_start;
    int previous_loop_end = vm->loop_end;

    UPDATE_SRCLOC(vm, condition);
    vm->loop_start = vm->chunk->ins_count;

    if (condition) {
        if (!compile_expression(vm, condition, true)) {
            return false;
        }
        EMIT_OP(vm, OP_JUMP_IF_FALSE);
        vm->loop_end = vm->chunk->ins_count - 1;
    } else {
        vm->loop_end = -1;
    }

    if (!compile_statement(vm, body)) {
        return false;
    }

    if (increment) {
        if (!compile_statement(vm, increment)) {
            return false;
        }
    }

    EMIT_OP_INT(vm, OP_JUMP, vm->loop_start - vm->chunk->ins_count - 1);

    if (condition) {
        vm->chunk->ins[vm->loop_end].value.intval = vm->chunk->ins_count - vm->loop_end - 1;
    }

    vm->loop_start = previous_loop_start;
    vm->loop_end = previous_loop_end;
    return true;
}

static bool compile_foreach(ApexVM *vm, AST *node) {
    UPDATE_SRCLOC(vm, node);
    AST *key_var = node->left; 
    AST *value_var = node->right;
    AST *iterable = node->value.ast_node->left;
    AST *body = node->value.ast_node->right;

    // Compile the iterable expression
    if (!compile_expression(vm, iterable, true)) {
        return false;
    }

    // Emit OP_ITER_START to initialize iteration
    EMIT_OP(vm, OP_ITER_START);

    // Record loop start
    int loop_start = vm->chunk->ins_count;

    // Emit OP_ITER_NEXT to fetch the next item
    EMIT_OP(vm, OP_ITER_NEXT);

    // Emit OP_JUMP_IF_DONE to check iteration end
    EMIT_OP(vm, OP_JUMP_IF_DONE);
    int loop_end = vm->chunk->ins_count - 1; // Address for later patching

    // Compile assignment of key and/or value
    if (key_var) {
        EMIT_OP_STR(vm, vm->in_function ? OP_SET_LOCAL : OP_SET_GLOBAL, key_var->value.strval);
    } else {
        EMIT_OP(vm, OP_POP);
    }
    if (value_var) {
        EMIT_OP_STR(vm, vm->in_function ? OP_SET_LOCAL : OP_SET_GLOBAL, value_var->value.strval);
    }

    // Compile the body of the loop
    if (!compile_statement(vm, body)) {
        return false;
    }

    // Emit a jump back to the loop start
    EMIT_OP_INT(vm, OP_JUMP, loop_start - vm->chunk->ins_count - 1);

    // Patch the OP_JUMP_IF_DONE to jump to after the loop
    vm->chunk->ins[loop_end].value.intval = vm->chunk->ins_count - loop_end - 1;

    return true;
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
static bool compile_statement(ApexVM *vm, AST *node) {
    if (!node) {
        return true;
    }

    switch (node->type) {
    case AST_IF: {
        int false_jmp_i;
        int true_jmp_i;

        UPDATE_SRCLOC(vm, node);

        if (!compile_expression(vm, node->left, true)) { // Condition
            return false;
        }
        EMIT_OP(vm, OP_JUMP_IF_FALSE);
        false_jmp_i = vm->chunk->ins_count - 1;

        if (!compile_statement(vm, node->right)) { // Block or statement 
            return false;
        }
        EMIT_OP(vm, OP_JUMP);
        true_jmp_i = vm->chunk->ins_count - 1;

        if (node->value.ast_node) {
            vm->chunk->ins[false_jmp_i].value.intval = vm->chunk->ins_count - false_jmp_i - 1;
            if (!compile_statement(vm, node->value.ast_node)) {
                return false;
            }
        } else {
            vm->chunk->ins[false_jmp_i].value.intval = vm->chunk->ins_count - false_jmp_i - 1;
        }
        vm->chunk->ins[true_jmp_i].value.intval = vm->chunk->ins_count - true_jmp_i - 1;
        break;
    }
    case AST_SWITCH:
        return compile_switch(vm, node);

    case AST_WHILE: 
        return compile_loop(vm, node->left, node->right, NULL);

    case AST_FOR: 
        if (!compile_statement(vm, node->left)) {
            return false;
        }
        return compile_loop(
            vm, node->right, 
            node->value.ast_node->right, 
            node->value.ast_node->left);

    case AST_FOREACH:
        return compile_foreach(vm, node);

    case AST_INCLUDE:
        return compile_include(vm, node);
        
    case AST_CONTINUE:
        if (vm->loop_start == -1) {
            apexErr_syntax(node->srcloc, "invalid 'continue' outside of loop");
            return false;
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_start - vm->chunk->ins_count - 1);
        break;

    case AST_BREAK:
        if (vm->loop_end == -1) {
            apexErr_syntax(node->srcloc,"invalid 'break' outside of loop");
            return false;
        }
        EMIT_OP_INT(vm, OP_JUMP, vm->loop_end - vm->chunk->ins_count - 1);
        break;

    case AST_FN_DECL:
        return compile_function_declaration(vm, node);

    case AST_RETURN:
        if (!compile_expression(vm, node->left, true)) {
            return false;
        }
        EMIT_OP(vm, OP_RETURN);
        return true;

    case AST_BLOCK:            
        if (!compile_statement(vm, node->right)) {
            return false;
        } else if (node->left) {
            return compile_statement(vm, node->left);
        }           
        return true;

    case AST_STATEMENT:
        if (node->left) {
            if (!compile_statement(vm, node->left)) {
                return false;
            }
        }
        if (node->right && node->right->type != AST_CASE) {
            if (!compile_statement(vm, node->right)) {
                return false;
            }
        }
        return true;

    default:
        return compile_expression(vm, node, false);
    }
    return true;
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
bool apexCode_compile(ApexVM *vm, AST *program) {
    if (program->left) {
        if (!compile_statement(vm, program->left)) {
            return false;
        }
    }
    
    if (program->right) {
        if (!compile_statement(vm, program->right)) {
            return false;
        }
    }    
    EMIT_OP(vm, OP_HALT);
    return true;
}
