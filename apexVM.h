#ifndef VM_H
#define VM_H

#define STACK_MAX 256
#define GLOBALS_MAX 256
#define CALL_STACK_MAX 128

#include <stdbool.h>
#include "apexVal.h"
#include "apexSym.h"
#include "apexLex.h"

/**
 * Represents an opcode in the bytecode.
 */
typedef enum {
    /**
     * Pushes an integer value onto the stack.
     * 5 produces OP_PUSH_INT with value 5.
     */
    OP_PUSH_INT,
    /**
     * Pushes a double-precision floating-point value onto the stack.
     * 5.0 produces OP_PUSH_DBL with value 5.0.
     */
    OP_PUSH_DBL,
    /**
     * Pushes a string value onto the stack.
     * "hello" produces OP_PUSH_STR with value "hello".
     */
    OP_PUSH_STR,
    /**
     * Pushes a boolean value onto the stack.
     * true produces OP_PUSH_BOOL with value true.
     */
    OP_PUSH_BOOL,   
   /**
     * Pushes a null value onto the stack.
     * null produces OP_PUSH_NULL.
     */
    OP_PUSH_NULL,
     /**
     * Creates a new array object and pushes it onto the stack.
     * [1, 2, 3].
     */
    OP_CREATE_ARRAY,
    /**
     * Sets the value of an array element.
     * a[0] = 5
     */
    OP_SET_ELEMENT,
    /**
     * Gets the value of an array element.
     * a[0]
     */
    OP_GET_ELEMENT,
    /**
     * Pops the top value off the stack.
     */
    OP_POP,
    /**
     * Adds two values together and pushes the result onto the stack.
     * a + b
     */
    OP_ADD,
    /**
     * Subtracts two values together and pushes the result onto the stack.
     * a - b .
     */
    OP_SUB,
    /**
     * Multiplies two values together and pushes the result onto the stack.
     * a * b
     */
    OP_MUL,
    /**
     * Divides two values together and pushes the result onto the stack.
     * a / b
     */
    OP_DIV,
    /**
     * Computes the remainder of two values together and pushes the result onto the stack.
     * a % b
     */
    OP_MOD,
    /**
     * Increments a local variable before its value is used in an expression.
     * ++a
     */
    OP_PRE_INC_LOCAL,
    /**
     * Increments a local variable after its value is used in an expression.
     * a++
     */
    OP_POST_INC_LOCAL,
    /**
     * Increments a global variable before its value is used in an expression.
     * ++a
     */
    OP_PRE_INC_GLOBAL,
    /**
     * Increments a global variable after its value is used in an expression.
     * a++
     */
    OP_POST_INC_GLOBAL,
    /**
     * Decrements a local variable before its value is used in an expression.
     * --a
     */
    OP_PRE_DEC_LOCAL,
    /**
     * Decrements a local variable after its value is used in an expression.
     * a--
     */
    OP_POST_DEC_LOCAL,
    /**
     * Decrements a global variable before its value is used in an expression.
     * --a
     */
    OP_PRE_DEC_GLOBAL,
    /**
     * Decrements a global variable after its value is used in an expression.
     * a--
     */
    OP_POST_DEC_GLOBAL,
    /**
     * Returns from a function.
     * return 5
     */
    OP_RETURN,
    /**
     * Calls a function or a closure.
     * foo(1, 2)
     */
    OP_CALL,
    /**
     * Starts an iteration over an array.
     * foreach (a in b) { ... }
     */
    OP_ITER_START,
    /**
     * Fetches the next item from an array iteration.
     * foreach (a in b) { ... }
     */
    OP_ITER_NEXT,
    /**
     * Jumps to an instruction.
     */
    OP_JUMP,
    /**
     * Jumps to an instruction if the top of the stack is false.
     * if (!a) { ... }
     */
    OP_JUMP_IF_FALSE,
    /**
     * Jumps to a label if the array iteration is done.
     * foreach (a in b) { ... }
     */
    OP_JUMP_IF_DONE,
    /**
     * Gets the value of a global variable.
     */
    OP_GET_GLOBAL,
    /**
     * Sets the value of a global variable.
     * foo = 5
     */
    OP_SET_GLOBAL,
    /**
     * Gets the value of a local variable.
     */
    OP_GET_LOCAL,
    /**
     * Sets the value of a local variable.
     * foo = 5
     */
    OP_SET_LOCAL,
    /**
     * Computes the logical NOT of the top of the stack.
     * !foo
     */
    OP_NOT,
    /**
     * Computes the negation of the top of the stack.
     * -foo
     */
    OP_NEGATE,
    /**
     * Computes the positive of the top of the stack.
     * +foo
     */
    OP_POSITIVE,
    /**
     * Calls a library function.
     * foo:bar()
     */
    OP_CALL_LIB,
    /**
     * Get the member of a library
     * foo:bar
     */
    OP_GET_LIB_MEMBER,
    /**
     * Starts a function definition.
     * foo() { ... }
     */
    OP_FUNCTION_START,
    /**
     * Signifies the end of a function definition.
     * foo() { ... }
     */
    OP_FUNCTION_END,
    /**
     * Compares two values for equality.
     * a == b
     */
    OP_EQ,
    /**
     * Compares two values for inequality.
     * a != b
     */
    OP_NE,
    /**
     * Compares two values for less than.
     * a < b
     */
    OP_LT,
    /**
     * Compares two values for less than or equal.
     * a <= b
     */
    OP_LE,
    /**
     * Compares two values for greater than.
     * a > b
     */
    OP_GT,
    /**
     * Compares two values for greater than or equal.
     * a >= b
     */
    OP_GE,
    /**
     * Creates a new object.
     * foo.new()
     */
    OP_NEW,
    /**
     * Sets the value of an object member.
     * foo.bar = 5
     */
    OP_SET_MEMBER,
    /**
     * Gets the value of an object member.
     * foo.bar
     */
    OP_GET_MEMBER,
    /**
     * Calls a function member.
     * foo.bar()
     */
    OP_CALL_MEMBER,
    /**
     * Creates a new object.
     * { foo = 5, bar = 10 }
     */
    OP_CREATE_OBJECT,
    /**
     * Creates a new closure.
     * fn(a, b) {}
     */
    OP_CREATE_CLOSURE,
    /**
     * Signifies the end of the VM execution.
     */
    OP_HALT
} OpCode;

/**
 * Represents a call frame in the call stack.
 */
typedef struct {
    const char *fn_name; /** Current function name */
    SrcLoc srcloc; /** Source location of the callframe */
} CallFrame;

/**
 * Represents an instruction in the bytecode.
 */
typedef struct {
    OpCode opcode; /** <Opcode of the instruction */
    ApexValue value; /** <Value associated with the instruction */
    SrcLoc srcloc; /** <Source location of the instruction */
} Ins;

/**
 * Represents a chunk of bytecode.
 */
typedef struct {
    Ins *ins; /** Chunk instructions */
    int ins_count; /** Number of instructions */
    int ins_size; /** Size of allocated instructions */
} Chunk;

/**
 * Represents the state of the virtual machine.
 */
typedef struct ApexVM {
    CallFrame call_stack[CALL_STACK_MAX]; /** Call stack */
    int call_stack_top; /** Top of the call stack */
    bool in_function; /** Whether the code is currently in a function */
    Chunk *chunk; /** Bytecode Chunk */
    Ins *ins; /** Pointer to the current instruction */
    ApexValue stack[STACK_MAX]; /** The value stack */
    ApexValue obj_context; /** Object context */
    int stack_top; /** Index of the stack top */
    int ip; /** Position of the instruction */
    int loop_start; /** Start of a loop */
    int loop_end; /** End of the loop */
    SrcLoc srcloc; /** Current source location of the vm */
    SymbolTable global_table; /** Global variable table */
    ScopeStack local_scopes; /** Local scopes containing each scoped symbol table */
} ApexVM;

extern void apexVM_pushval(ApexVM *vm, ApexValue value);
extern void apexVM_pushstr(ApexVM *vm, ApexString *str);
extern void apexVM_pushint(ApexVM *vm, int i);
extern void apexVM_pushflt(ApexVM *vm, float flt);
extern void apexVM_pushdbl(ApexVM *vm, double dbl);
extern void apexVM_pushbool(ApexVM *vm, bool b);
extern void apexVM_pusharr(ApexVM *vm, ApexArray *arr);
extern void apexVM_pushnull(ApexVM *vm);
extern ApexValue apexVM_pop(ApexVM *vm);
extern ApexValue apexVM_peek(ApexVM *vm, int offset);
extern bool apexVM_call(ApexVM *vm, ApexFn *fn, int argc);
extern void print_vm_instructions(ApexVM *vm);
extern void init_vm(ApexVM *vm);
extern void apexVM_reset(ApexVM *vm);
extern void free_vm(ApexVM *vm);
extern bool vm_dispatch(ApexVM *vm);

#endif
