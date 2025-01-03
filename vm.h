#ifndef VM_H
#define VM_H

#define STACK_MAX 256
#define GLOBALS_MAX 256
#define CALL_STACK_MAX 128

#include "value.h"
#include "symbol.h"
#include "bool.h"
#include "srcloc.h"

typedef enum {
    OP_PUSH_INT,
    OP_PUSH_FLT,
    OP_PUSH_STR,
    OP_PUSH_BOOL,   
    OP_CREATE_ARRAY,
    OP_SET_ELEMENT,
    OP_GET_ELEMENT,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_RETURN,
    OP_CALL,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_NOT,
    OP_NEGATE,
    OP_CALL_LIB,
    OP_FUNCTION_START,
    OP_FUNCTION_END,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_HALT
} OpCode;

typedef struct {
    const char *fn_name;
    SrcLoc srcloc;
} CallFrame;

typedef struct {
    OpCode opcode;
    ApexValue value;
    SrcLoc srcloc;
} Ins;

typedef struct {
    Ins *ins;
    int ins_n;
    int ins_size;
} Chunk;

typedef struct {
    CallFrame call_stack[CALL_STACK_MAX];
    int call_stack_top;
    Bool in_function;
    Chunk *chunk;
    Ins *ins;
    ApexValue stack[STACK_MAX];
    int stack_top;
    int ip;
    int loop_start;
    int loop_end;
    SrcLoc srcloc;
    SymbolTable fn_table;
    SymbolTable global_table;
    ScopeStack local_scopes;
} ApexVM;

extern void apexVM_pushval(ApexVM *vm, ApexValue value);
extern void apexVM_pushstr(ApexVM *vm, const char *str);
extern void apexVM_pushint(ApexVM *vm, int i);
extern void apexVM_pushflt(ApexVM *vm, float flt);
extern void apexVM_pushbool(ApexVM *vm, Bool b);
extern ApexValue apexVM_pop(ApexVM *vm);
extern void print_vm_instructions(ApexVM *vm);
extern void init_vm(ApexVM *vm);
extern void free_vm(ApexVM *vm);
extern void vm_dispatch(ApexVM *vm);

#endif
