#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "apexVM.h"
#include "apexSym.h"
#include "apexMem.h"
#include "apexStr.h"
#include "apexErr.h"
#include "apexLib.h"
#include "apexParse.h"

#define STACK_PUSH(vm, val) ((vm)->stack[(vm)->stack_top++] = (val))
#define STACK_POP(vm)       ((vm)->stack[--(vm)->stack_top])

/**
 * @brief Converts an opcode to its string representation
 *
 * @param opcode The opcode to convert
 *
 * @return A string representing the opcode
 */
static const char *opcode_to_string(OpCode opcode) {
    switch (opcode) {
        case OP_PUSH_INT: return "OP_PUSH_INT";
        case OP_PUSH_DBL: return "OP_PUSH_DBL";
        case OP_PUSH_STR: return "OP_PUSH_STR";
        case OP_PUSH_BOOL: return "OP_PUSH_BOOL";
        case OP_PUSH_NULL: return "OP_PUSH_NULL";
        case OP_CREATE_ARRAY: return "OP_CREATE_ARRAY";
        case OP_SET_ELEMENT: return "OP_SET_ELEMENT";
        case OP_GET_ELEMENT: return "OP_GET_ELEMENT";
        case OP_POP: return "OP_POP";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";
        case OP_PRE_INC_LOCAL: return "OP_PRE_INC_LOCAL";
        case OP_POST_INC_LOCAL: return "OP_POST_INC_LOCAL";
        case OP_PRE_INC_GLOBAL: return "OP_PRE_INC_GLOBAL";
        case OP_POST_INC_GLOBAL: return "OP_POST_INC_GLOBAL";
        case OP_PRE_DEC_LOCAL: return "OP_PRE_DEC_LOCAL";
        case OP_POST_DEC_LOCAL: return "OP_POST_DEC_LOCAL";
        case OP_PRE_DEC_GLOBAL: return "OP_PRE_DEC_GLOBAL";
        case OP_POST_DEC_GLOBAL: return "OP_POST_DEC_GLOBAL";
        case OP_RETURN: return "OP_RETURN";
        case OP_CALL: return "OP_CALL";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_JUMP_IF_DONE: return "OP_JUMP_IF_DONE";
        case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";    
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_ITER_START: return "OP_ITER_START";
        case OP_ITER_NEXT: return "OP_ITER_NEXT";
        case OP_NOT: return "OP_NOT";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_POSITIVE: return "OP_POSITIVE";
        case OP_CALL_LIB: return "OP_CALL_LIB";
        case OP_FUNCTION_START: return "OP_FUNCTION_START";
        case OP_FUNCTION_END: return "OP_FUNCTION_END";
        case OP_EQ: return "OP_EQ";
        case OP_NE: return "OP_NE";
        case OP_LT: return "OP_LT";
        case OP_LE: return "OP_LE";
        case OP_GT: return "OP_GT";
        case OP_GE: return "OP_GE";
        case OP_GET_LIB_MEMBER: return "OP_GET_LIB_MEMBER";
        case OP_SET_MEMBER: return "OP_SET_MEMBER";
        case OP_GET_MEMBER: return "OP_GET_MEMBER";
        case OP_CALL_MEMBER: return "OP_CALL_MEMBER";
        case OP_CREATE_OBJECT: return "OP_CREATE_OBJECT";
        case OP_CREATE_CLOSURE: return "OP_CREATE_CLOSURE";
        case OP_NEW: return "OP_NEW";
        case OP_HALT: return "OP_HALT";
    }
    return "Unknown opcode";
}

/**
 * Prints out the instructions in the ApexVM's instruction chunk to the console.
 *
 * This function is only compiled when the DEBUG flag is defined. It prints out
 * each instruction in the instruction chunk, including the opcode and any
 * associated value. The opcode is printed as a string, and the value is
 * printed according to its type (int, double, string, boolean). The function
 * is useful for debugging the ApexVM.
 *
 * @param vm A pointer to the ApexVM containing the instruction chunk.
 */
void print_vm_instructions(ApexVM *vm) {
    printf("== ApexVM Instructions ==\n");
    for (int i = 0; i < vm->chunk->ins_count; i++) {
        Ins *ins = &vm->chunk->ins[i];
        printf("%04d: %-20s", i, opcode_to_string(ins->opcode));
        
        switch (ins->opcode) {
        case OP_PUSH_INT:
            printf("%d", ins->value.intval);
            break;

        case OP_PUSH_DBL:
            printf("%f", ins->value.dblval);
            break;

        case OP_PUSH_STR:
            printf("\"%s\"", ins->value.strval->value);
            break;

        case OP_PUSH_BOOL:
            printf("%s", ins->value.boolval ? "true" : "false");
            break;
        
        case OP_CALL:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP:
        case OP_SET_LOCAL:
            printf("%d", ins->value.intval);
            break;

        case OP_SET_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_GET_LOCAL:
        case OP_SET_MEMBER:
            printf("\"%s\"", ins->value.strval->value);
            break;

        default:
            break;
        }
        printf("\n");
    }
}

static bool vm_execute(ApexVM *vm, Ins *ins);

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
    vm->in_function = false;
    vm->chunk = apexMem_alloc(sizeof(Chunk));
    vm->chunk->ins = apexMem_alloc(sizeof(Ins) * 8);
    vm->chunk->ins_size = 8;
    vm->chunk->ins_count = 0;
    vm->loop_start = -1;
    vm->loop_end = -1;
    vm->srcloc.lineno = 0;
    vm->srcloc.filename = NULL;
    vm->call_stack_top = 0;
    vm->obj_context = apexVal_makenull();
    init_symbol_table(&vm->global_table);
    init_scope_stack(&vm->local_scopes);
}

/**
 * Resets the virtual machine's instruction chunk.
 *
 * This function frees the current instruction memory and reallocates it
 * with an initial size of 8 instructions. It also resets the instruction
 * count to zero, effectively clearing any previously loaded instructions.
 *
 * @param vm A pointer to the virtual machine structure to reset.
 */
void apexVM_reset(ApexVM *vm) {
    free(vm->chunk->ins);
    vm->chunk->ins = apexMem_alloc(sizeof(Ins) * 8);
    vm->chunk->ins_size = 8;
    vm->chunk->ins_count = 0;
    vm->ip = 0;
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
    free_scope_stack(&vm->local_scopes);
}

/**
 * Creates a call frame structure.
 *
 * This function creates a call frame with the given function name, source
 * location, and object context. The returned call frame is suitable for
 * being pushed onto the call stack.
 *
 * @param fn_name The name of the function being called.
 * @param srcloc The source location of the call site.
 * @param obj_context The object context of the call.
 * @return A call frame structure with the given information.
 */
static CallFrame create_callframe(const char *fn_name, SrcLoc srcloc) {
    CallFrame callframe;
    callframe.fn_name = fn_name;
    callframe.srcloc = srcloc;
    return callframe;
}

/**
 * Pushes a new call frame onto the virtual machine's call stack.
 *
 * This function creates a call frame with the specified function name,
 * source location, and object context, and adds it to the top of the
 * call stack. If the call stack is full, an error is raised and the
 * program is terminated.
 *
 * @param vm A pointer to the virtual machine structure.
 * @param fn_name The name of the function for the new call frame.
 * @param srcloc The source location for the new call frame.
 * @param obj_context The object context for the new call frame.
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
static CallFrame pop_callframe(ApexVM *vm, SrcLoc srcloc) {
    if (vm->call_stack_top == 0) {
        apexErr_fatal(srcloc, "call stack underflow!");
    }
    return vm->call_stack[--vm->call_stack_top];
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
        apexErr_fatal(vm->srcloc, "stack overflow\n");
    }
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
        apexErr_fatal(vm->srcloc, "stack underflow");
    }
    value = vm->stack[--vm->stack_top];
    return value;
}

static ApexValue stack_top(ApexVM *vm) {
    return vm->stack[vm->stack_top - 1];
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
ApexValue apexVM_peek(ApexVM *vm, int offset) {
    if (vm->stack_top - 1 - offset < 0) {
        apexErr_fatal(vm->srcloc, "stack underflow: invalid offset");
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
void apexVM_pushstr(ApexVM *vm, ApexString *str) {
    stack_push(vm, apexVal_makestr(str));
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
 * Pushes a double value onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param dbl The double value to push.
 */
void apexVM_pushdbl(ApexVM *vm, double dbl) {
    stack_push(vm, apexVal_makedbl(dbl));
}

/**
 * Pushes a boolean value onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param b The boolean value to push.
 */
void apexVM_pushbool(ApexVM *vm, bool b) {
    stack_push(vm, apexVal_makebool(b));
}

/**
 * Pushes an array value onto the stack.
 *
 * This function takes a pointer to an ApexVM and an ApexArray, creates an
 * ApexValue with the array type from the given array, and pushes the value
 * onto the stack.
 *
 * @param vm A pointer to the virtual machine to use.
 * @param arr The ApexArray to push.
 */
void apexVM_pusharr(ApexVM *vm, ApexArray *arr) {
    stack_push(vm, apexVal_makearr(arr));
}

/**
 * Pushes a null value onto the virtual machine's stack.
 *
 * This function creates an ApexValue representing a null value and pushes
 * it onto the stack of the specified virtual machine instance. The function
 * does not perform any type conversion or checking on the value being pushed.
 *
 * @param vm A pointer to the virtual machine instance whose stack the
 *           null value will be pushed onto.
 */
void apexVM_pushnull(ApexVM *vm) {
    stack_push(vm, apexVal_makenull());
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
 * Calls an Apex function with the given number of arguments.
 *
 * This function first verifies that the number of arguments provided is
 * valid for the given function. If the function takes a variable number of
 * arguments, it then verifies that the number of arguments provided is
 * at least equal to the number of fixed parameters in the function. If the
 * number of arguments is invalid, it raises a runtime error and returns
 * false.
 *
 * After verifying the number of arguments, this function allocates a new
 * scope and adds the function's parameters to the scope. If the function
 * takes a variable number of arguments, it collects all arguments after the
 * last fixed parameter into an ApexArray and adds it to the scope. All
 * arguments are then pushed onto the scope's stack.
 *
 * Finally, this function sets the virtual machine's instruction pointer to
 * the address of the given function, and pushes the current instruction
 * pointer onto the stack as the return address.
 *
 * @param vm A pointer to the virtual machine to call the function in.
 * @param fn A pointer to the Apex function to call.
 * @return true if the function was called successfully, false if an error
 *         occurred.
 */
bool apexVM_call(ApexVM *vm, ApexFn *fn, int argc) {
    int ret_addr = vm->ip;
            
    if (argc != fn->argc) {
        apexErr_runtime(vm, "expected %d arguments, got %d", fn->argc, argc);
        return false;
    }

    push_callframe(vm, fn->name, vm->ins->srcloc);
    push_scope(&vm->local_scopes);  

    for (int i = 0; i < argc; i++) {      
        ApexValue value = stack_pop(vm);
        apexSym_setlocal(
            &vm->local_scopes,
            fn->params[i],
            value);        
    }
    stack_push(vm, apexVal_makeint(ret_addr));
    vm->ip = fn->addr;     
    Ins *ins; 
    do {
        ins = next_ins(vm);
        if (!vm_execute(vm, ins)) {
            return false;
        }
    } while (ins->opcode != OP_RETURN);
    return true;
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
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval + b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.intval + b.dblval);
    }

    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.fltval + b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval + b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.fltval + b.dblval);
    }

    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.dblval + b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        return apexVal_makedbl(a.dblval + b.intval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        return apexVal_makedbl(a.dblval + b.fltval);
    }
  
    if (a.type == APEX_VAL_STR && b.type == APEX_VAL_STR) {
        ApexString *newstr = apexStr_cat(a.strval, b.strval);
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
        apexErr_runtime(vm, "cannot add string to a flt");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_DBL) ||
        (a.type == APEX_VAL_DBL && b.type == APEX_VAL_STR)) {
        apexErr_runtime(vm, "cannot add string to a dbl");
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
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval - b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.intval - b.dblval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.fltval - b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval - b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.fltval - b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.dblval - b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        return apexVal_makedbl(a.dblval - b.intval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        return apexVal_makedbl(a.dblval - b.fltval);
    }
    apexErr_runtime(
        vm, "cannot subtract %s from %s", 
        apexVal_typestr(a), 
        apexVal_typestr(b));
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
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval * b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.intval * b.dblval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.fltval * b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval * b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.fltval * b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        return apexVal_makedbl(a.dblval * b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        return apexVal_makedbl(a.dblval * b.intval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        return apexVal_makedbl(a.dblval * b.fltval);
    }
    apexErr_runtime(
        vm, "cannot multiply %s with %s", 
        apexVal_typestr(a), 
        apexVal_typestr(b));
    return apexVal_makenull();
}

/**
 * Checks if the value of `x` is zero, and if so, raises a runtime error
 * and returns a null value.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param x The value to check.
 */
#define CHECK_DIV_ZERO(vm, x) do { \
    if (x == 0) { \
        apexErr_runtime(vm, "division by zero"); \
        return apexVal_makenull(); \
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
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(vm, b.fltval);
        return apexVal_makeflt(a.intval / b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        CHECK_DIV_ZERO(vm, b.dblval);
        return apexVal_makedbl(a.intval / b.dblval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(vm, b.fltval);
        return apexVal_makeflt(a.fltval / b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(vm, b.intval);
        return apexVal_makeflt(a.fltval / b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        CHECK_DIV_ZERO(vm, b.dblval);
        return apexVal_makedbl(a.fltval / b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        CHECK_DIV_ZERO(vm, b.dblval);
        return apexVal_makedbl(a.dblval / b.dblval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(vm, b.intval);
        return apexVal_makedbl(a.dblval / b.intval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(vm, b.fltval);
        return apexVal_makedbl(a.dblval / b.fltval);
    }
    apexErr_runtime(
        vm, "cannot divide %s by %s", 
        apexVal_typestr(a), 
        apexVal_typestr(b));
    return apexVal_makenull();
}

/**
 * Checks if the value of `x` is zero, and if so, raises a runtime error
 * and returns a null value.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param x The value to check.
 */
#define CHECK_MOD_ZERO(vm, x) do { \
    if (x == 0) { \
        apexErr_runtime(vm, "modulus by zero"); \
        return apexVal_makenull(); \
    } \
} while (0)

/**
 * Computes the modulus of two values, `a` and `b`, and returns the result as
 * an ApexValue.
 *
 * The modulus operator can be used on both integers and floats. If the
 * modulus results in a remainder, the remainder is the result of the
 * operation.
 *
 * If the types of `a` and `b` are incompatible for modulus, an
 * error is raised and a null value is returned.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param a The first operand as an ApexValue.
 * @param b The second operand as an ApexValue.
 * @return The result of the modulus as an ApexValue.
 */
static ApexValue vm_mod(ApexVM *vm, ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        CHECK_MOD_ZERO(vm, b.intval);        
        return apexVal_makeint(a.intval % b.intval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        CHECK_MOD_ZERO(vm, b.fltval);
        return apexVal_makeflt(a.intval % (int)roundf(b.fltval));
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        CHECK_MOD_ZERO(vm, b.dblval);
        return apexVal_makedbl(a.intval % (int)round(b.dblval));
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        CHECK_MOD_ZERO(vm, b.fltval);
        return apexVal_makeflt((int)roundf(a.fltval) % (int)roundf(b.fltval));
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        CHECK_MOD_ZERO(vm, b.intval);
        return apexVal_makeflt((int)roundf(a.fltval) / b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        CHECK_MOD_ZERO(vm, b.dblval);
        return apexVal_makedbl((int)roundf(a.fltval) / (int)round(b.dblval));
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        CHECK_MOD_ZERO(vm, b.dblval);
        return apexVal_makedbl((int)round(a.dblval) / (int)round(b.dblval));
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        CHECK_MOD_ZERO(vm, b.intval);
        return apexVal_makedbl((int)round(a.dblval) / b.intval);
    }
    if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        CHECK_MOD_ZERO(vm, b.fltval);
        return apexVal_makedbl((int)round(a.dblval) / (int)roundf(b.fltval));
    }
    apexErr_runtime(
        vm, "cannot apply modulus on %s by %s", 
        apexVal_typestr(a), 
        apexVal_typestr(b));
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
 * the comparison defaults to false.
 *
 * @param a The first ApexValue to compare.
 * @param b The second ApexValue to compare.
 * @param opcode The operation code representing the comparison to perform.
 * 
 * @return An ApexValue of type boolean representing the result of the comparison.
 */
static ApexValue vm_cmp(ApexVM *vm, ApexValue a, ApexValue b, OpCode opcode) {
#define CMP_NUMS() do { \
    switch (opcode) { \
    case OP_EQ: result = left == right; break; \
    case OP_NE: result = left != right; break; \
    case OP_LT: result = left < right; break; \
    case OP_LE: result = left <= right; break; \
    case OP_GT: result = left > right; break; \
    case OP_GE: result = left >= right; break; \
    default: result = false; break; \
    } \
} while (0)
    bool result = false;
    if ((a.type != APEX_VAL_INT &&
         a.type != APEX_VAL_FLT &&
         a.type != APEX_VAL_DBL) ||
        (b.type != APEX_VAL_INT &&
         b.type != APEX_VAL_FLT &&
         b.type != APEX_VAL_DBL)) {
        if (opcode == OP_LT || 
            opcode == OP_LE || 
            opcode == OP_GT || 
            opcode == OP_GE) {
            apexErr_runtime(vm, 
                "cannot compare %s to %s", 
                apexVal_typestr(a), 
                apexVal_typestr(b));
            return apexVal_makenull();
        }
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        int left = a.intval;
        int right = b.intval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        float left = a.intval;
        float right = b.fltval;
       CMP_NUMS();
    } else if (a.type == APEX_VAL_INT && b.type == APEX_VAL_DBL) {
        double left = a.intval;
        double right = b.dblval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        float left = a.fltval;
        float right = b.fltval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        float left = a.fltval;
        float right = b.intval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_DBL) {
        double left = a.fltval;
        double right = b.dblval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_DBL) {
        double left = a.dblval;
        double right = b.dblval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_INT) {
        double left = a.dblval;
        double right = b.intval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_DBL && b.type == APEX_VAL_FLT) {
        double left = a.dblval;
        double right = b.fltval;
        CMP_NUMS();
    } else if (a.type != b.type) {
        result = false;
    } else if (a.type == APEX_VAL_BOOL) {
        if (opcode == OP_EQ) {
            result = a.boolval == b.boolval;
        } else if (opcode == OP_NE) {
            result = a.boolval != b.boolval;
        } else {
            result = false;
        }        
    } else if (a.type == APEX_VAL_STR) {
        if (opcode == OP_EQ) {
            result = a.strval->value == b.strval->value;
        } else if (opcode == OP_NE) {
            result = a.strval->value != b.strval->value;
        } else {
            result = false;
        }
    } else if (a.type == APEX_VAL_NULL) {
        if (opcode == OP_EQ) {
            result = true;
        } else if (opcode == OP_NE) {
            result = false;
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
 * Increments the value of the given ApexValue.
 *
 * This function increments the value of the given ApexValue, handling
 * integers, floats, and doubles.
 *
 * If the value is not a numeric type, an error is raised.
 *
 * @param vm The ApexVM containing the value to increment.
 * @param value The ApexValue to increment.
 * @return True on success, false on error.
 */
static bool incvalue(ApexVM *vm, ApexValue *value) {
    switch (value->type) {
    case APEX_VAL_INT:
        value->intval++;
        break;
    case APEX_VAL_FLT:
        value->fltval++;
        break;
    case APEX_VAL_DBL:
        value->dblval++;
        break;
    default:
        apexErr_runtime(vm, "cannot increment %s", apexVal_typestr(*value));
        return false;
    }
    return true;
}

/**
 * Decrements the value of the given ApexValue.
 *
 * This function decrements the value of the given ApexValue, handling
 * integers, floats, and doubles.
 *
 * If the value is not a numeric type, an error is raised.
 *
 * @param vm The ApexVM containing the value to decrement.
 * @param value The ApexValue to decrement.
 * @return True on success, false on error.
 */
static bool decvalue(ApexVM *vm, ApexValue *value) {
    switch (value->type) {
    case APEX_VAL_INT:
        value->intval--;
        break;
    case APEX_VAL_FLT:
        value->fltval--;
        break;
    case APEX_VAL_DBL:
        value->dblval--;
        break;
    default:
        apexErr_runtime(vm, "cannot decrement %s", apexVal_typestr(*value));
        return false;
    }
    return true;
}

/**
 * Executes a single instruction on the virtual machine.
 *
 * This function processes a single instruction (`ins`) from the instruction
 * chunk associated with the virtual machine (`vm`). It performs the operation
 * specified by the instruction's opcode and manipulates the virtual machine's
 * stack and state accordingly. The function handles various opcodes for
 * pushing and popping values, arithmetic operations, function calls, control
 * flow, and other virtual machine instructions.
 *
 * The function returns `true` if the instruction execution is successful,
 * and `false` if an error occurs during execution, such as an invalid
 * operation or runtime exception.
 *
 * @param vm A pointer to the ApexVM structure representing the virtual machine.
 * @param ins A pointer to the instruction to be executed.
 * @return `true` on successful execution, or `false` if an error occurs.
 */
static bool vm_execute(ApexVM *vm, Ins *ins) {
    switch (ins->opcode) {
    case OP_PUSH_INT:
    case OP_PUSH_DBL:
    case OP_PUSH_STR:
    case OP_PUSH_BOOL:
        stack_push(vm, ins->value);
        break;

    case OP_PUSH_NULL:
        stack_push(vm, apexVal_makenull());
        break;

    case OP_POP:
        stack_pop(vm);
        break;

    case OP_ADD: {
        ApexValue value = vm_add(vm, stack_pop(vm), stack_pop(vm));
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;        
    }
    case OP_SUB: {
        ApexValue value = vm_sub(vm, stack_pop(vm), stack_pop(vm));
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;        
    }
    case OP_MUL: {
        ApexValue value = vm_mul(vm, stack_pop(vm), stack_pop(vm));
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;        
    }
    case OP_DIV: {
        ApexValue value = vm_div(vm, stack_pop(vm), stack_pop(vm));
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;        
    }
    case OP_MOD: {
        ApexValue value = vm_mod(vm, stack_pop(vm), stack_pop(vm));
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;
    }
    case OP_RETURN: {
        ApexValue ret_val;
        int ret_addr = 0;
        CallFrame frame = pop_callframe(vm, ins->srcloc);
        if (vm->obj_context.type == APEX_VAL_OBJ && 
            frame.fn_name == apexStr_new("new", 3)->value) {
            ApexValue obj_context = vm->obj_context;                
            if (vm->stack_top == 1) {
                ret_addr = stack_pop(vm).intval;
            } else if (vm->stack_top > 1) {
                apexErr_error(vm->srcloc, "warning: return value of 'new' is discarded'");
                stack_pop(vm);
                ret_addr = stack_pop(vm).intval;
            }                
            stack_push(vm, obj_context);
        } else {
            if (vm->stack_top == 1) {
                ret_addr = stack_pop(vm).intval;                
            } else if (vm->stack_top > 1) {
                ret_val = stack_pop(vm);
                ret_addr = stack_pop(vm).intval;
                stack_push(vm, ret_val);
            }
        }
        vm->obj_context = apexVal_makenull();
        vm->ip = ret_addr;
        pop_scope(&vm->local_scopes);
        break;
    }
    case OP_CALL: {
        ApexValue fnval = stack_pop(vm);
        int argc = ins->value.intval;

        if (fnval.type == APEX_VAL_CFN) {
            ApexCfn cfn = fnval.cfnval;               
            if (cfn.fn(vm, argc) != 0) {
                return false;
            }
            break;
        }

        int ret_addr = vm->ip;
        ApexFn *fn = fnval.fnval;
        
        if (fn->have_variadic) {
            if (argc < fn->argc - 1) {
                apexErr_runtime(vm,
                    "expected at least %d arguments, got %d",
                    fn->argc, argc);
                return false;
            }                
        } else if (argc != fn->argc) {
            apexErr_runtime(vm, 
                "expected %d arguments, got %d", 
                fn->argc, argc);
            return false;
        }

        push_callframe(vm, fn->name, ins->srcloc);
        push_scope(&vm->local_scopes);

        ApexArray *variadic_args = NULL;
        int param_index = 0;
        bool have_variadic = false;

        if (fn->have_variadic) {
            variadic_args = apexVal_newarray();
        }

        int variadic_index = argc - fn->argc;

        for (int i = 0; i < argc; i++) {
            if (fn->have_variadic && argc - i >= fn->argc) {
                apexVal_arrayset(
                    variadic_args,
                    apexVal_makeint(variadic_index--),
                    stack_pop(vm));
                have_variadic = true;
            } else {
                if (have_variadic) {
                    apexSym_setlocal(
                        &vm->local_scopes, 
                        fn->params[param_index++], 
                        apexVal_makearr(variadic_args));
                    have_variadic = false;
                }
                ApexValue value = stack_pop(vm);
                apexSym_setlocal(
                    &vm->local_scopes,
                    fn->params[param_index++],
                    value);
            }
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
        if (!apexVal_isassigned(condition)) {
            apexVal_retain(condition);
        }
        if (!apexVal_tobool(condition)) {  
            if (!apexVal_isassigned(condition)) {
                apexVal_release(condition); 
            }                            
            vm->ip += ins->value.intval;
        }            
        break;
    }
    case OP_JUMP_IF_DONE: {
        ApexValue condition = stack_pop(vm);
        if (!condition.boolval) {
            ApexValue iterable = stack_pop(vm);
            if (!apexVal_isassigned(iterable)) {
                apexVal_release(iterable);
            }
            vm->ip += ins->value.intval;
        }
        break;
    }
    case OP_ITER_START: {            
        ApexValue iterable = stack_pop(vm);
        if (iterable.type == APEX_VAL_ARR) {
             if (!apexVal_isassigned(iterable)) {
                apexVal_retain(iterable);
            }
            stack_push(vm, apexVal_makeint(0)); // Push initial index
            stack_push(vm, iterable);          // Push iterable itself
        } else {
            apexErr_runtime(vm, "foreach requires an array");
            return false;
        }
        break;
    }
    case OP_ITER_NEXT: {
        ApexValue iterable = stack_pop(vm);
        ApexValue index = stack_pop(vm);

        if (iterable.type != APEX_VAL_ARR) {
            apexErr_runtime(vm, "invalid iterable type in foreach");
            return false;
        }

        // Check if iteration is complete
        if (index.intval >= iterable.arrval->iter_count) {
            stack_push(vm, iterable); 
            stack_push(vm, apexVal_makebool(false)); // Signal iteration end
        } else {
            // Push current value and advance index
            ApexArrayEntry *entry = iterable.arrval->iter[index.intval];
            
            stack_push(vm, apexVal_makeint(index.intval + 1)); // Next index
            stack_push(vm, iterable);                
            stack_push(vm, entry->value);
            stack_push(vm, entry->key);
            stack_push(vm, apexVal_makebool(true)); // Signal iteration continues
        }
        break;
    }
    case OP_CREATE_ARRAY: {
        ApexArray *array = apexVal_newarray();
        int i = vm->stack_top - ins->value.intval * 2;
        while (i < vm->stack_top - 1) {                
            ApexValue key = vm->stack[i];
            ApexValue value = vm->stack[i + 1];                  
            apexVal_arrayset(array, key, value);
            i += 2;
        }
        vm->stack_top -= ins->value.intval * 2;
        stack_push(vm, apexVal_makearr(array));
        break;
    }
    case OP_GET_ELEMENT: { // array[index]
        ApexValue index = stack_pop(vm);
        ApexValue array = stack_pop(vm);          
        switch (array.type) {
        case APEX_VAL_STR: {
            ApexString *str = array.strval;
            if (index.intval >= (int)str->len) {
                apexErr_runtime(vm, "index out of bounds: %d", index.intval);
                return false;
            }
            char chstr[1];                
            snprintf(chstr, 2, "%c", str->value[index.intval]);
            str = apexStr_new(chstr, 1);
            stack_push(vm, apexVal_makestr(str));
            break;
        }
        case APEX_VAL_ARR:
            ApexValue value;
            if (!apexVal_arrayget(&value, array.arrval, index)) {
                char *indexstr = apexVal_tostr(index)->value;
                apexErr_runtime(vm, "invalid array index: %s", indexstr);
                return false;
            }
            stack_push(vm, value);
            break;            
        default:
            if (array.type != APEX_VAL_ARR) {
                apexErr_runtime(vm, 
                    "cannot index non-array value: %s", 
                    apexVal_typestr(array));
                return false;
            }
        }           
        break;
    }
    case OP_SET_ELEMENT: { // array[index] = value
        ApexValue index = stack_pop(vm);            
        ApexValue array = stack_pop(vm);
        ApexValue value = stack_pop(vm);            
        apexVal_arrayset(array.arrval, index, value);
        break;
    }
    case OP_NEW: { // object.new()
        ApexValue objval = stack_pop(vm);
        ApexObject *obj = objval.objval;
        ApexValue newFnVal;
        
        if (apexVal_objectget(&newFnVal, obj, apexStr_new("new", 3)->value)) {
            int ret_addr = vm->ip;
            ApexFn *fn = newFnVal.fnval;
            int argc = ins->value.intval;
            if (fn->have_variadic) {
                if (argc < fn->argc) {
                    apexErr_runtime(vm, "expected at least %d arguments, got %d", fn->argc, argc);
                    return false;
                }
            } else if (argc != fn->argc) {
                apexErr_runtime(vm, "expected %d arguments, got %d", fn->argc, argc);
                return false;
            }
            push_callframe(vm, fn->name, ins->srcloc);
            push_scope(&vm->local_scopes);

            ApexArray *variadic_args = NULL;
            int param_index = 0;
            bool have_variadic = false;

            if (fn->have_variadic) {
                variadic_args = apexVal_newarray();
            }
            int variadic_index = argc - fn->argc;
            for (int i = 0; i < argc; i++) {
                if (fn->have_variadic && argc - i >= fn->argc) {
                    apexVal_arrayset(variadic_args, apexVal_makeint(variadic_index--), stack_pop(vm));
                    have_variadic = true;
                } else {
                    if (have_variadic) {
                        apexSym_setlocal(&vm->local_scopes, fn->params[param_index++], apexVal_makearr(variadic_args));
                        have_variadic = false;
                    }
                    apexSym_setlocal(&vm->local_scopes, fn->params[param_index++], stack_pop(vm));
                }
            }
            ApexObject *newobj = apexVal_objectcpy(obj);                
            vm->obj_context = apexVal_makeobj(newobj);
            stack_push(vm, apexVal_makeint(ret_addr));
            vm->ip = fn->addr;
        } else {
            if (ins->value.intval > 0) {
                apexErr_runtime(vm, "expected 0 arguments, got %d", vm->stack_top);
                return false;
            }
            ApexObject *newobj = apexVal_objectcpy(obj);
            stack_push(vm, apexVal_makeobj(newobj));
        }        
        break;
    }
    case OP_CREATE_OBJECT: {
        ApexValue objval;
        ApexValue name = stack_pop(vm);
        if (!apexSym_getglobal(&objval, &vm->global_table, name.strval->value)) {
            apexErr_runtime(vm, "object '%s' not defined", name.strval->value);
            return false;
        }
        ApexObject *obj = objval.objval;
        for (int i = ins->value.intval; i; i--) {
            ApexValue value = stack_pop(vm);
            ApexValue key = stack_pop(vm);            
            apexVal_objectset(obj, key.strval->value, value);
        }            
        break;
    }
    case OP_GET_MEMBER: {
        ApexValue objval = stack_pop(vm);
        ApexObject *obj = objval.objval;
        ApexValue value;

        if (objval.type != APEX_VAL_OBJ && objval.type != APEX_VAL_TYPE) {
            apexErr_runtime(vm, "attempt to get field '%s' on non object", ins->value.strval->value);
            return false;
        }

        if (!apexVal_objectget(&value, obj, ins->value.strval->value)) {
            apexErr_runtime(vm, 
                "object '%s' has no field '%s'", 
                obj->name, ins->value.strval->value);
            return false;
        }

        stack_push(vm, value);
        break;
    }
    case OP_CALL_MEMBER: { // obj.method(arg1, arg2, ...)
        char *name = ins->value.strval->value;
        int argc = stack_pop(vm).intval;
        ApexValue objval = stack_top(vm);
        ApexValue fnval;

        if (!apexVal_objectget(&fnval, objval.objval, name)) {
            apexErr_runtime(vm, "object '%s' has no field '%s'", objval.objval->name, name);
            return false;
        }

        if (fnval.type == APEX_VAL_CFN) {
            ApexCfn cfn = fnval.cfnval;
            if (cfn.fn(vm, argc) != 0) {
                return false;
            }
            if (vm->stack_top > 0 && stack_top(vm).type == APEX_VAL_OBJ) {
                stack_pop(vm);
            }
            break;
        }
        ApexFn *fn = fnval.fnval;
        int ret_addr = vm->ip;
        stack_pop(vm);
        if (fn->have_variadic) {
            if (argc < fn->argc) {
                apexErr_runtime(vm, "expected at least %d arguments, got %d", fn->argc, argc);
                return false;
            }
        } else if (argc != fn->argc) {
            apexErr_runtime(vm, "expected %d arguments, got %d", fn->argc, argc);
            return false;
        }

        push_callframe(vm, fn->name, ins->srcloc);
        push_scope(&vm->local_scopes);       

        ApexArray *variadic_args = NULL;
        int param_index = 0;
        bool have_variadic = false;

        if (fn->have_variadic) {
            variadic_args = apexVal_newarray();
        }
        int variadic_index = argc - fn->argc;
        for (int i = 0; i < argc; i++) {
            if (fn->have_variadic && argc - i >= fn->argc) {
                apexVal_arrayset(
                    variadic_args,
                    apexVal_makeint(variadic_index--),
                    stack_pop(vm));
                have_variadic = true;
            } else {
                if (have_variadic) {
                    apexSym_setlocal(
                        &vm->local_scopes,
                        fn->params[param_index++],
                        apexVal_makearr(variadic_args));
                    have_variadic = false;
                }
                apexSym_setlocal(
                    &vm->local_scopes,
                    fn->params[param_index++],
                    stack_pop(vm));
            }
        }       
        vm->obj_context = objval;
        stack_push(vm, apexVal_makeint(ret_addr));
        vm->ip = fn->addr;
        break;
    }
    case OP_SET_MEMBER: { // obj.field = value
        ApexValue objval = stack_pop(vm);
        ApexValue value = stack_pop(vm);
        apexVal_objectset(objval.objval, ins->value.strval->value, value);
        break;
    }
    case OP_SET_GLOBAL: { // var = value
        const char *name = ins->value.strval->value;
        ApexValue value = stack_pop(vm);
        apexSym_setglobal(&vm->global_table, name, value);
        break;
    }
    case OP_GET_GLOBAL: {
        const char *name = ins->value.strval->value;
        ApexValue value;
        if (!apexSym_getglobal(&value, &vm->global_table, name)) {
            apexErr_runtime(vm, "global variable '%s' not found", name);
            return false;
        }
        stack_push(vm, value);
        break;
    }
    case OP_SET_LOCAL: { // var = value
        const char *name = ins->value.strval->value;
        ApexValue value = stack_pop(vm);            
        apexSym_setlocal(&vm->local_scopes, name, value);
        break;
    }
    case OP_GET_LOCAL: {
        ApexString *name = ins->value.strval;
        ApexValue value;

        if (name == apexStr_new("this", 4)) {
            if (vm->obj_context.type == APEX_VAL_NULL) {
                apexErr_runtime(vm, "cannot access 'this' outside of object context");
                return false;
            }
            stack_push(vm, vm->obj_context);
            break;
        }

        if (!apexSym_getlocal(&value, &vm->local_scopes, name->value)) {
            apexErr_runtime(vm, "local variable '%s' not found", name->value);
            return false;
        }
        stack_push(vm, value);
        break;
    }
    case OP_CREATE_CLOSURE: {
        ApexValue value = ins->value;
        stack_push(vm, value);
        break;
    }

    #define GET_ARRAY_DATA \
        ApexValue index = stack_pop(vm); \
        ApexValue array = stack_pop(vm); \
        ApexValue value; \
        if (!apexVal_arrayget(&value, array.arrval, index)) { \
            char *indexstr = apexVal_tostr(index)->value; \
            apexErr_runtime(vm, "invalid array index: %s", indexstr); \
            return false; \
        }

    case OP_PRE_INC_LOCAL: { // ++local_var
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, value);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getlocal(&value, &vm->local_scopes, name)) {
                apexErr_runtime(vm, "local variable '%s' not found", name);
                return false;
            }
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexSym_setlocal(&vm->local_scopes, name, value);      
            stack_push(vm, value);
        }                   
        break;
    }
    case OP_POST_INC_LOCAL: { // local_var++
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            ApexValue prev = value;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, prev);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getlocal(&value, &vm->local_scopes, name)) {
                apexErr_runtime(vm, "local variable '%s' not found", name);
                return false;
            }   
            ApexValue prev = value;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexSym_setlocal(&vm->local_scopes, name, value);
            stack_push(vm, prev);
        }
        break;
    }
    case OP_PRE_DEC_LOCAL: { // --local_var
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, value);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getlocal(&value, &vm->local_scopes, name)) {
                apexErr_runtime(vm, "local variable '%s' not found", name);
                return false;
            }  
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexSym_setlocal(&vm->local_scopes, name, value);
            stack_push(vm, value);
        }
        break;
    }
    case OP_POST_DEC_LOCAL: {  // local_var--
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            ApexValue prev = value;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, prev);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getlocal(&value, &vm->local_scopes, name)) {
                apexErr_runtime(vm, "local variable '%s' not found", name);
                return false;
            }           
            ApexValue prev = value;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexSym_setlocal(&vm->local_scopes, name, value);
            stack_push(vm, prev);
        }
        break;
    }
    case OP_PRE_INC_GLOBAL: { // ++global_var
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, value);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getglobal(&value, &vm->global_table, name)) {
                apexErr_runtime(vm, "global variable '%s' not found", name);
                return false;
            }
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexSym_setglobal(&vm->global_table, name, value);
            stack_push(vm, value);
        }
        break;
    }
    case OP_POST_INC_GLOBAL: { // global_var++
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            ApexValue prev = value;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, prev);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getglobal(&value, &vm->global_table, name)) {
                apexErr_runtime(vm, "global variable '%s' not found", name);
                return false;
            }
            ApexValue prev = value;
            if (!incvalue(vm, &value)) {
                return false;
            }
            apexSym_setglobal(&vm->global_table, name, value);
            stack_push(vm, prev);
        }
        break;
    }
    case OP_PRE_DEC_GLOBAL: { // --global_var
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, value);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getglobal(&value, &vm->global_table, name)) {
                apexErr_runtime(vm, "global variable '%s' not found", name);
                return false;
            }
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexSym_setglobal(&vm->global_table, name, value);
            stack_push(vm, value);
            break;
        }
    }
    case OP_POST_DEC_GLOBAL: { // global_var--
        if (vm->stack_top > 1) {
            GET_ARRAY_DATA;
            ApexValue prev = value;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexVal_arrayset(array.arrval, index, value);
            stack_push(vm, prev);
        } else {
            const char *name = ins->value.strval->value;
            ApexValue value;
            if (!apexSym_getglobal(&value, &vm->global_table, name)) {
                apexErr_runtime(vm, "global variable '%s' not found", name);
                return false;
            }
            ApexValue prev = value;
            if (!decvalue(vm, &value)) {
                return false;
            }
            apexSym_setglobal(&vm->global_table, name, value);
            stack_push(vm, prev);
        }
        break;
    }
    case OP_NOT: { // !
        ApexValue value = stack_pop(vm);
        bool boolval = apexVal_tobool(value);
        stack_push(vm, apexVal_makebool(!boolval));
        break;
    }
    case OP_NEGATE: {
        ApexValue val = stack_pop(vm);
        if (val.type == APEX_VAL_INT) {
            stack_push(vm, apexVal_makeint(-val.intval));
        } else if (val.type == APEX_VAL_FLT) {
            stack_push(vm, apexVal_makeflt(-val.fltval));
        } else if (val.type == APEX_VAL_DBL) {
            stack_push(vm, apexVal_makedbl(-val.dblval));
        } else {
            apexErr_runtime(vm, "cannot negate %s", apexVal_typestr(val));
            return false;
        }
        break;
    }
    case OP_POSITIVE: {
        ApexValue val = stack_pop(vm);
        if (val.type == APEX_VAL_INT) {
            stack_push(vm, apexVal_makeint(+val.intval));
        } else if (val.type == APEX_VAL_FLT) {
            stack_push(vm, apexVal_makeflt(+val.fltval));
        } else if (val.type == APEX_VAL_DBL) {
            stack_push(vm, apexVal_makedbl(+val.dblval));
        } else {
            apexErr_runtime(vm, "cannot positive %s", apexVal_typestr(val));
            return false;
        }
        break;
    }
    case OP_CALL_LIB: {
        ApexValue fn_name_val = stack_pop(vm);
        ApexValue lib_name_val = stack_pop(vm);
        const char *lib_name = lib_name_val.strval->value;
        const char *fn_name = fn_name_val.strval->value;
        ApexLibData lib_data = apexLib_get(lib_name, fn_name);
        if (!lib_data.name || lib_data.is_var) {
            apexErr_runtime(vm, "undefined library function '%s:%s'", lib_name, fn_name);
            return false;
        }
        int argc = ins->value.intval;            
        if (lib_data.fn(vm, argc) == 1) {
            return false;
        }
        break;
    }
    case OP_GET_LIB_MEMBER: {
        ApexValue member_name_val = stack_pop(vm);
        ApexValue lib_name_val = stack_pop(vm);
        char *lib_name = lib_name_val.strval->value;
        char *member_name = member_name_val.strval->value;
        ApexLibData lib_data = apexLib_get(lib_name, member_name);
        if (!lib_data.name || !lib_data.is_var) {
            apexErr_runtime(vm, "undefined library member '%s:%s'", lib_name, member_name);
            return false;
        }
        apexVM_pushval(vm, *lib_data.var);
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
        ApexValue value = vm_cmp(vm, stack_pop(vm), stack_pop(vm), ins->opcode);
        if (value.type == APEX_VAL_NULL) {
            return false;
        }
        stack_push(vm, value);
        break;
    }
    case OP_HALT:
        return true;        
    }
    return true;
}

/**
 * Executes the instructions in the virtual machine's instruction chunk.
 *
 * This function runs indefinitely until an error occurs or the program
 * explicitly halts.
 *
 * @param vm A pointer to the virtual machine structure containing the
 *           instruction chunk.
 * @return true if the program executed successfully, false if an error
 *         occurred.
 */
bool vm_dispatch(ApexVM *vm) {
    Ins *ins;
    do {
        ins = next_ins(vm);
        if (!vm_execute(vm, ins)) {
            return false;
        }
    } while (ins->opcode != OP_HALT);
    return true;
}