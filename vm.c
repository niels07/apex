#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "symbol.h"
#include "mem.h"
#include "bool.h"
#include "string.h"
#include "error.h"
#include "stdlib.h"
#include "parser.h"

#define STACK_PUSH(vm, val) ((vm)->stack[(vm)->stack_top++] = (val))
#define STACK_POP(vm)       ((vm)->stack[--(vm)->stack_top])

static const char *opcode_to_string(OpCode opcode) {
    switch (opcode) {
        case OP_PUSH_INT: return "OP_PUSH_INT";
        case OP_PUSH_FLT: return "OP_PUSH_FLT";
        case OP_PUSH_STR: return "OP_PUSH_STR";
        case OP_PUSH_BOOL: return "OP_PUSH_BOOL";
        case OP_CREATE_ARRAY: return "OP_CREATE_ARRAY";
        case OP_SET_ELEMENT: return "OP_SET_ELEMENT";
        case OP_GET_ELEMENT: return "OP_GET_ELEMENT";
        case OP_POP: return "OP_POP";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_RETURN: return "OP_RETURN";
        case OP_CALL: return "OP_CALL";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";    
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_NOT: return "OP_NOT";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_CALL_LIB: return "OP_CALL_LIB";
        case OP_FUNCTION_START: return "OP_FUNCTION_START";
        case OP_FUNCTION_END: return "OP_FUNCTION_END";
        case OP_EQ: return "OP_EQ";
        case OP_NE: return "OP_NE";
        case OP_LT: return "OP_LT";
        case OP_LE: return "OP_LE";
        case OP_GT: return "OP_GT";
        case OP_GE: return "OP_GE";
        case OP_HALT: return "OP_HALT";
    }
    return "Unknown opcode";
}

void print_vm_instructions(ApexVM *vm) {
    int i;
    printf("== ApexVM Instructions ==\n");
    for (i = 0; i < vm->chunk->ins_n; i++) {
        Ins *instr = &vm->chunk->ins[i];
        printf("%04d: %-20s", i, opcode_to_string(instr->opcode));
        
        switch (instr->opcode) {
        case OP_PUSH_INT:
            printf("%d", instr->value.intval);
            break;

        case OP_PUSH_FLT:
            printf("%f", instr->value.fltval);
            break;

        case OP_PUSH_STR:
            printf("\"%s\"", instr->value.strval);
            break;

        case OP_PUSH_BOOL:
            printf("%s", instr->value.boolval ? "true" : "false");
            break;
        
        case OP_CALL:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP:
        case OP_SET_LOCAL:
            printf("%d", instr->value.intval);
            break;

        case OP_SET_GLOBAL:
        case OP_GET_GLOBAL:
            printf("\"%s\"", instr->value.strval);
            break;

        case OP_GET_LOCAL:
            printf("%d", instr->value.intval);
            break;

        default:
            break;
        }
        printf("\n");
    }
}

/**
 * Initializes a virtual machine structure.
 *
 * This function initializes all the fields of a virtual machine structure. It
 * allocates memory for the instruction chunk and the global and function
 * symbol tables, and initializes the local scope stack. It also sets the
 * instruction pointer and stack top to 0, and sets the loop start and end
 * pointers to -1.
 *
 * @param vm A pointer to the virtual machine structure to be initialized.
 */
void init_vm(ApexVM *vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    vm->in_function = FALSE;
    vm->chunk = mem_alloc(sizeof(Chunk));
    vm->chunk->ins = mem_alloc(sizeof(Ins) * 8);
    vm->chunk->ins_size = 8;
    vm->chunk->ins_n = 0;
    vm->loop_start = -1;
    vm->loop_end = -1;
    vm->srcloc.lineno = 0;
    vm->srcloc.filename = NULL;
    vm->call_stack_top = 0;
    init_symbol_table(&vm->global_table);
    init_symbol_table(&vm->fn_table);
    init_scope_stack(&vm->local_scopes);
}

/**
 * Frees the memory allocated for the virtual machine structure.
 *
 * This function should be called when a virtual machine is no longer needed. It
 * frees all memory allocated for the instruction chunk, the global symbol table,
 * the function table, and the local scope stack.
 *
 * @param vm A pointer to the virtual machine structure to be freed.
 */
void free_vm(ApexVM *vm) {
    free(vm->chunk->ins);
    free(vm->chunk);
    free_symbol_table(&vm->global_table);
    free_symbol_table(&vm->fn_table);
    free_scope_stack(&vm->local_scopes);
}


/**
 * Creates a CallFrame structure with the given function name and source location.
 *
 * This function is used to create a call frame structure for the call stack.
 * It does not check if the function name is valid or if the source location is
 * valid. It simply copies the given values into a CallFrame structure and
 * returns it.
 *
 * @param fn_name The name of the function for the call frame.
 * @param srcloc The source location for the call frame.
 * @return A CallFrame structure with the given function name and source
 *         location.
 */
static CallFrame create_callframe(const char *fn_name, SrcLoc srcloc) {
    CallFrame callframe;
    callframe.fn_name = fn_name;
    callframe.srcloc = srcloc;
    return callframe;
}

/**
 * Pushes a callframe onto the call stack with the given function name and source
 * location.
 *
 * This function checks if the call stack is full before pushing a new callframe
 * onto the call stack. If the call stack is full, it will raise a runtime error
 * of "call stack overflow". Otherwise, it will allocate a new callframe with the
 * given function name and source location, and push it onto the call stack.
 *
 * @param vm A pointer to the virtual machine instance whose call stack the
 *           callframe will be pushed onto.
 * @param fn_name The name of the function for the callframe.
 * @param srcloc The source location for the callframe.
 */
static void push_callframe(ApexVM *vm, const char *fn_name, SrcLoc srcloc) {
    if (vm->call_stack_top >= CALL_STACK_MAX) {
        apexErr_fatal(srcloc, "Call stack overflow!");
    }
    vm->call_stack[vm->call_stack_top++] = create_callframe(fn_name, srcloc);
}

/**
 * Pops the top call frame off the call stack.
 *
 * This function checks if the call stack is empty before popping a call frame
 * off the call stack. If the call stack is empty, it will raise a runtime error
 * of "call stack underflow". Otherwise, it will decrement the call stack top
 * and return.
 *
 * @param vm A pointer to the virtual machine instance whose call stack the
 *           callframe will be popped from.
 * @param srcloc The source location for the error message if the call stack is
 *               underflow.
 */
static void pop_callframe(ApexVM *vm, SrcLoc srcloc) {
    if (vm->call_stack_top == 0) {
        apexErr_fatal(srcloc, "call stack underflow!");
    }
    vm->call_stack_top--;
}


/**
 * Increments the reference count of a value if it is an array.
 *
 * If the value is not an array, this function does nothing.
 *
 * @param value The value to increment the reference count of.
 */
static void increment_reference(ApexValue value) {
    if (value.type == APEX_VAL_ARR) {
        value.refcount++;
    }
}

/**
 * Decrements the reference count of a value if it is an array, and frees
 * the array if the reference count reaches zero.
 *
 * This function checks if the given value is of type array. If it is, it
 * decrements the reference count. If the reference count becomes zero or
 * negative, the memory allocated for the array is freed.
 *
 * @param value The value whose reference count will be decremented.
 */
static void decrement_reference(ApexValue value) {
    if (value.type == APEX_VAL_ARR) {
        value.refcount--;
        if (value.refcount <= 0) {
            apexVal_freearray(value.arrval);          
        }
    }
}

/**
 * Retrieves the next instruction from the virtual machine's instruction chunk.
 *
 * This function moves the instruction pointer to the next instruction in the
 * instruction chunk, and returns a pointer to the instruction located at the
 * new instruction pointer.
 *
 * @param vm A pointer to the virtual machine instance whose instruction chunk
 *           the instruction will be retrieved from.
 * @return A pointer to the next instruction in the instruction chunk.
 */
static Ins *next_ins(ApexVM *vm) {
    vm->ins = &vm->chunk->ins[vm->ip++];
    return vm->ins;
}

/**
 * Pushes an ApexValue onto the virtual machine's stack.
 *
 * This function takes an ApexValue and pushes it onto the stack of the
 * specified virtual machine instance. It does not perform any type checking
 * of the value. If the stack is full (i.e. the stack top points to the end
 * of the stack array), it will generate a runtime error of "stack overflow".
 *
 * @param vm A pointer to the virtual machine instance whose stack the
 *           value will be pushed onto.
 * @param value The ApexValue to be pushed onto the stack.
 */
static void stack_push(ApexVM *vm, ApexValue value) {
    if (vm->stack_top >= STACK_MAX) {
        apexErr_runtime(vm, "Stack overflow\n");
    }
    increment_reference(value);
    vm->stack[vm->stack_top++] = value;
}

/**
 * Pops an ApexValue off the virtual machine's stack.
 *
 * This function decrements the stack top and returns the value at the new
 * top of the stack. If the stack is empty (i.e. the top points to the
 * beginning of the stack), it will generate a runtime error of "stack
 * underflow".
 *
 * @param vm A pointer to the virtual machine instance whose stack the
 *           value will be popped from.
 *
 * @return The ApexValue at the top of the stack after popping.
 */
static ApexValue stack_pop(ApexVM *vm) {
    ApexValue value;
    if (vm->stack_top == 0) {
        apexErr_runtime(vm, "stack underflow");
    }
    value = vm->stack[--vm->stack_top];
    return value;
}

/**
 * Peeks at an ApexValue on the virtual machine's stack.
 *
 * This function takes a virtual machine instance and an offset, and returns
 * the value at the given offset from the top of the stack without modifying
 * the stack. If the offset is invalid (i.e. it points to a location before the
 * beginning of the stack), it will generate a runtime error of "stack
 * underflow: invalid offset".
 *
 * @param vm A pointer to the virtual machine instance whose stack the
 *           value will be peeked from.
 * @param offset The offset from the top of the stack to peek from.
 *
 * @return The value at the given offset from the top of the stack.
 */
ApexValue stack_peek(ApexVM *vm, int offset) {
    if (vm->stack_top - 1 - offset < 0) {
        apexErr_runtime(vm, "stack underflow: invalid offset");
    }
    return vm->stack[vm->stack_top - 1 - offset];
}

/**
 * Pushes an ApexValue onto the virtual machine's stack.
 *
 * This function takes an ApexValue and pushes it onto the stack of the 
 * specified virtual machine instance. It does not perform any type 
 * conversion or checking on the value being pushed.
 *
 * @param vm A pointer to the virtual machine instance whose stack the
 *           value will be pushed onto.
 * @param value The ApexValue to push onto the stack.
 */

void apexVM_pushval(ApexVM *vm, ApexValue value) {
    stack_push(vm, value);
}

/**
 * Pushes a string value onto the stack.
 *
 * This function allocates a new string from the string table and creates an
 * ApexValue with the string type from the newly allocated string. The string
 * value is then pushed onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param str The string value to push.
 */
void apexVM_pushstr(ApexVM *vm, const char *str) {
    char *s = apexStr_new(str, strlen(str));
    stack_push(vm, apexVal_makestr(s));
}

/**
 * Pushes an integer value onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param i The integer value to push.
 */
void apexVM_pushint(ApexVM *vm, int i) {
    stack_push(vm, apexVal_makeint(i));
}

/**
 * Pushes a float value onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param flt The float value to push.
 */
void apexVM_pushflt(ApexVM *vm, float flt) {
    stack_push(vm, apexVal_makeflt(flt));
}

/**
 * Pushes a boolean value onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param b The boolean value to push.
 */
void apexVM_pushbool(ApexVM *vm, Bool b) {
    stack_push(vm, apexVal_makebool(b));
}

static void set_array_element(Array *array, ApexValue index, ApexValue value) {
    ApexValue existing_value;

    if (apexVal_arrayget(&existing_value, array, index)) {
        decrement_reference(existing_value);
    }

    increment_reference(value);
    apexVal_arrayset(array, index, value);
}

ApexValue get_array_element(ApexVM *vm, Array *array, ApexValue index) {
    ApexValue value;

    if (!apexVal_arrayget(&value, array, index)) {
        char *indexstr = apexStr_valtostr(index);
        apexErr_runtime(vm, "invalid array index: %s", indexstr);
    }

    return value;
}

/**
 * Pops the top value off the stack and returns it.
 *
 * This function is equivalent to a call to stack_pop, and is provided as a
 * convenience. It does not perform any error checking or bounds checking on
 * the stack.
 *
 * @return The top value on the stack.
 */
ApexValue apexVM_pop(ApexVM *vm) {
    return stack_pop(vm);
}

/**
 * Adds two ApexValue objects and returns the result.
 *
 * This function performs addition between two values, `a` and `b`. It supports
 * addition of integers and floats, promoting integers to floats when necessary.
 * It also concatenates two strings if both values are strings.
 *
 * If the types of `a` and `b` are incompatible for addition, such as a boolean
 * with any other type, an error is raised. Specifically, strings cannot be
 * added to integers, floats, or booleans, and booleans cannot be added to any
 * numeric type.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param a The first operand as an ApexValue.
 * @param b The second operand as an ApexValue.
 * @return The result of the addition as an ApexValue, or null if an error occurs.
 */
static ApexValue vm_add(ApexVM *vm, ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval + b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        ApexValue v = apexVal_makeflt(a.fltval + b.fltval);
        return v;
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval + b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval + b.intval);
    }
    if (a.type == APEX_VAL_STR && b.type == APEX_VAL_STR) {
        char *newstr = cat_string(b.strval, a.strval);
        return apexVal_makestr(newstr);
    }
    if (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_BOOL) {
        apexErr_runtime(vm, "cannot perform arithmetic on a boolean value");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_INT) || 
        (a.type == APEX_VAL_INT && b.type == APEX_VAL_STR)) {
        apexErr_runtime(vm, "cannot add string to an int");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_FLT) || 
        (a.type == APEX_VAL_FLT && b.type == APEX_VAL_STR)) {
        apexErr_runtime(vm, "cannot add string to an flt");
    }
    if ((a.type == APEX_VAL_INT && b.type == APEX_VAL_BOOL) || 
        (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_INT)) {
        apexErr_runtime(vm, "cannot add bool to an int");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_BOOL) || 
        (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_STR)) {
        apexErr_runtime(vm, "cannot add string to a bool");
    }
    return apexVal_makenull();
}

/**
 * Subtracts one value from another. If the two values are both integers, or
 * both floats, or one integer and one float, the result is a float. If the
 * two values are not both numbers, an error is raised.
 *
 * @param vm The virtual machine on which to perform the operation.
 * @param a The value from which to subtract.
 * @param b The value to subtract.
 * @return The result of the subtraction.
 */
static ApexValue vm_sub(ApexVM *vm,ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval - b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.fltval - b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval - b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval - b.intval);
    }
    apexErr_runtime(
        vm, "cannot subtract %s from %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

/**
 * Multiplies two ApexValue objects and returns the result.
 *
 * This function performs multiplication between two values, `a` and `b`.
 * It supports multiplication of integers and floats, and handles mixed
 * type operations by promoting integers to floats when necessary.
 * 
 * If the types of `a` and `b` are incompatible for multiplication, an
 * error is raised and a null value is returned.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param a The first operand as an ApexValue.
 * @param b The second operand as an ApexValue.
 * @return The result of the multiplication as an ApexValue.
 */
static ApexValue vm_mul(ApexVM *vm, ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval * b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        ApexValue v = apexVal_makeflt(a.fltval * b.fltval);
        return v;
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval * b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval * b.intval);
    }
    apexErr_runtime(
        vm, "cannot multiply %s with %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

#define CHECK_DIV_ZERO(vm, x) do { \
    if (x == 0) { \
        apexErr_runtime(vm, "division by zero"); \
    } \
} while (0)

/**
 * Divides the value on top of the stack by the value below it.
 *
 * The division operator can be used on both integers and floats. If the
 * division results in a remainder, the remainder is discarded.
 *
 * @return The result of division as an ApexValue.
 */
static ApexValue vm_div(ApexVM *vm, ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(vm, b.intval);
        return apexVal_makeint(a.intval / b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(vm, b.fltval);
        return apexVal_makeflt(a.fltval / b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(vm, b.fltval);
        return apexVal_makeflt(a.intval / b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(vm, b.intval);
        return apexVal_makeflt(a.fltval / b.intval);
    }
    apexErr_runtime(
        vm, "cannot divide %s by %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

/**
 * Compares two ApexValue objects based on the given opcode.
 *
 * This function performs comparisons between two values, `a` and `b`, 
 * depending on their types and the provided comparison operation code.
 * It supports comparison of integers, floats, booleans, strings, and 
 * function pointers. The comparison result is returned as an ApexValue 
 * of type boolean.
 *
 * Supported opcodes for comparison include:
 * - OP_EQ: Checks if the two values are equal.
 * - OP_NE: Checks if the two values are not equal.
 * - OP_LT: Checks if the first value is less than the second value.
 * - OP_LE: Checks if the first value is less than or equal to the second value.
 * - OP_GT: Checks if the first value is greater than the second value.
 * - OP_GE: Checks if the first value is greater than or equal to the second value.
 *
 * If the values are of incompatible types for the given operation, 
 * the comparison defaults to FALSE.
 *
 * @param a The first ApexValue to compare.
 * @param b The second ApexValue to compare.
 * @param opcode The operation code representing the comparison to perform.
 * 
 * @return An ApexValue of type boolean representing the result of the comparison.
 */
static ApexValue cmp(ApexValue a, ApexValue b, OpCode opcode) {
#define CMP_NUMS() do { \
    switch (opcode) { \
    case OP_EQ: result = left == right; break; \
    case OP_NE: result = left != right; break; \
    case OP_LT: result = left < right; break; \
    case OP_LE: result = left <= right; break; \
    case OP_GT: result = left > right; break; \
    case OP_GE: result = left >= right; break; \
    default: result = FALSE; break; \
    } \
} while (0)
    Bool result = FALSE;
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        int left = a.intval;
        int right = b.intval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        float left = a.fltval;
        float right = b.fltval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        float left = a.intval;
        float right = b.fltval;
       CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        float left = a.fltval;
        float right = b.intval;
        CMP_NUMS();
    } else if (a.type != b.type) {
        result = FALSE;
    } else if (a.type == APEX_VAL_BOOL) {
        if (opcode == OP_EQ) {
            result = a.boolval == b.boolval;
        } else if (opcode == OP_NE) {
            result = a.boolval != b.boolval;
        } else {
            result = FALSE;
        }        
    } else if (a.type == APEX_VAL_STR) {
        if (opcode == OP_EQ) {
            result = a.strval == b.strval;
        } else if (opcode == OP_NE) {
            result = a.strval != b.strval;
        } else {
            result = FALSE;
        }
    } else if (a.type == APEX_VAL_NULL) {
        if (opcode == OP_EQ) {
            result = TRUE;
        } else if (opcode == OP_NE) {
            result = FALSE;
        }
    } else if (a.type == APEX_VAL_FN) {
        if (opcode == OP_EQ) {
            result = a.fnval == b.fnval;
        } else if (opcode == OP_NE) {
            result = a.fnval != b.fnval;
        }
    }
    return apexVal_makebool(result);
}

/**
 * Runs the virtual machine.
 *
 * This function dispatches instructions from the current chunk of bytecode
 * until the OP_HALT instruction is encountered. It pushes and pops values
 * from the stack as necessary, and maintains a callstack of function calls.
 *
 * @param vm The ApexVM to dispatch instructions for.
 */
void vm_dispatch(ApexVM *vm) {
    for (;;) {
        Ins *ins = next_ins(vm);
        
        switch (ins->opcode) {
        case OP_PUSH_INT:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_FLT:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_STR:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_BOOL:
            stack_push(vm, ins->value);
            break;

        case OP_POP:
            stack_pop(vm);
            break;

        case OP_ADD: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_add(vm, a, b));
            break;
        }
        case OP_SUB: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_sub(vm, a, b));
            break;
        }

        case OP_MUL: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_mul(vm, a, b));
            break;
        }
        case OP_DIV:  {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_div(vm, a, b));
            break;
        }

        case OP_RETURN: {
            ApexValue ret_val;
            int ret_addr;

            pop_callframe(vm, ins->srcloc);
            ret_val = stack_pop(vm);
            if (vm->stack_top > 0) {
                ret_addr = stack_pop(vm).intval;
            } else {
                ret_addr = ret_val.intval;
                ret_val = apexVal_makenull();
            }

            
            vm->ip = ret_addr;
            stack_push(vm, ret_val);
            pop_scope(&vm->local_scopes);
            break;
        }

        case OP_CALL: {
            int fn_addr = ins->value.intval;
            int ret_addr = vm->ip;
            Fn *fn;
            int i;

            fn = get_symbol_value_by_addr(&vm->fn_table, fn_addr).fnval;
            push_callframe(vm, fn->name, ins->srcloc);
            push_scope(&vm->local_scopes);

            for (i = fn->param_n - 1; i >= 0; i--) {
                ApexValue arg = stack_pop(vm);
                const char *name = fn->params[i];
                SymbolAddr addr = add_local_symbol(&vm->local_scopes, name);
                set_local_symbol_value(&vm->local_scopes, addr, arg);
            }

            stack_push(vm, apexVal_makeint(ret_addr));
            vm->ip = fn->addr;
            break;
        }
        case OP_JUMP:
            vm->ip += ins->value.intval;
            break;

        case OP_JUMP_IF_FALSE: {
            ApexValue condition = stack_pop(vm);
            switch (condition.type) {
            case APEX_VAL_INT:
                condition = apexVal_makebool(condition.intval != 0);
                break;

            case APEX_VAL_FLT:
                condition = apexVal_makebool(condition.fltval != 0);
                break;

            case APEX_VAL_BOOL:
                break;

            case APEX_VAL_STR:
                condition = apexVal_makebool(condition.strval != NULL);
                break;

            case APEX_VAL_FN:
            case APEX_VAL_ARR:
                condition = apexVal_makebool(TRUE);
                break;

            case APEX_VAL_NULL:
                condition = apexVal_makebool(FALSE);
                break;
            }

            if (!condition.boolval) {
                vm->ip += ins->value.intval;
            }
            break;
        }
        case OP_CREATE_ARRAY: {
            int i;
            Array *array = apexVal_newarray();

            for (i = ins->value.intval; i; i--) {
                ApexValue value = stack_pop(vm);
                ApexValue key = stack_pop(vm);
               
                apexVal_arrayset(array, key, value);
            }
            stack_push(vm, apexVal_makearr(array));
            break;
        }
        case OP_GET_ELEMENT: {
            ApexValue index = stack_pop(vm);
            ApexValue array = stack_pop(vm);
            ApexValue value = get_array_element(vm, array.arrval, index);

            increment_reference(value);
            stack_push(vm, value);
            break;
        }
        case OP_SET_ELEMENT: {
            ApexValue value = stack_pop(vm);
            ApexValue index = stack_pop(vm);
            ApexValue array = stack_pop(vm);

            set_array_element(array.arrval, index, value);
            break;
        }
        case OP_SET_GLOBAL: {
            const char *name = ins->value.strval;
            ApexValue new_value = stack_pop(vm);
            ApexValue old_value = get_symbol_value_by_name(&vm->global_table, name);

            decrement_reference(old_value);
            increment_reference(new_value);

            if (!set_symbol_value(&vm->global_table, name, new_value)) {
                apexErr_fatal(ins->srcloc, "attemt to set an undefined global variable: '%s'", name);
            }
            break;
        }
        case OP_GET_GLOBAL: {
            const char *name = ins->value.strval;
            ApexValue value = get_symbol_value_by_name(&vm->global_table, name);
            increment_reference(value); 
            stack_push(vm, value);
            break;
        }
        case OP_SET_LOCAL: {
            int addr = ins->value.intval;
            ApexValue new_value = stack_pop(vm);
            ApexValue old_value = get_local_symbol_value(&vm->local_scopes, addr);

            decrement_reference(old_value);
            increment_reference(new_value);

            if (set_local_symbol_value(&vm->local_scopes, addr, new_value)) {
                apexErr_fatal(ins->srcloc, "attempt to set an undefined local variable: %d", addr);
            }
            break;
        }
        case OP_GET_LOCAL: {
            int addr = ins->value.intval;
            ApexValue value = get_local_symbol_value(&vm->local_scopes, addr);
            stack_push(vm, value);
            break;
        }
        case OP_NOT:
            stack_push(vm, apexVal_makebool(!stack_pop(vm).boolval));
            break;

        case OP_NEGATE: {
            ApexValue val = stack_pop(vm);

            if (val.type == APEX_VAL_INT) {
                stack_push(vm, apexVal_makeint(-val.intval));
            } else if (val.type == APEX_VAL_FLT) {
                stack_push(vm, apexVal_makeflt(-val.fltval));
            } else {
                apexErr_runtime(vm, "cannot negate %s", get_value_type_str(val.type));
            }
            break;
        }

        case OP_CALL_LIB: {
            int fn_addr = ins->value.intval;
            apex_stdlib[fn_addr].fn(vm);
            break;
        }

        case OP_FUNCTION_START:
             while (vm->chunk->ins[vm->ip].opcode != OP_FUNCTION_END) {
                vm->ip++;
            }
            vm->ip++;
            break; 

        case OP_FUNCTION_END:            
            break;

        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE: {
            ApexValue value = cmp(stack_pop(vm), stack_pop(vm), ins->opcode);
            stack_push(vm, value);
            break;
        }
        case OP_HALT:
            return;
            
        default:
            apexErr_fatal(ins->srcloc, "Unknown opcode: %d\n", ins->opcode);
        }
        
    }
}