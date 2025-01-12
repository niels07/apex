#ifndef VM_H
#define VM_H

#define STACK_MAX 256
#define GLOBALS_MAX 256
#define CALL_STACK_MAX 128

#include <stdbool.h>
#include "apexVal.h"
#include "apexSym.h"
#include "apexLex.h"

typedef enum {
    OP_PUSH_INT,
    OP_PUSH_DBL,
    OP_PUSH_STR,
    OP_PUSH_BOOL,   
    OP_PUSH_NULL,
    OP_CREATE_ARRAY,
    OP_SET_ELEMENT,
    OP_GET_ELEMENT,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_PRE_INC_LOCAL,
    OP_POST_INC_LOCAL,
    OP_PRE_INC_GLOBAL,
    OP_POST_INC_GLOBAL,
    OP_PRE_DEC_LOCAL,
    OP_POST_DEC_LOCAL,
    OP_PRE_DEC_GLOBAL,
    OP_POST_DEC_GLOBAL,
    OP_RETURN,
    OP_CALL,
    OP_ITER_START,
    OP_ITER_NEXT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_DONE,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_NOT,
    OP_NEGATE,
    OP_POSITIVE,
    OP_CALL_LIB,
    OP_FUNCTION_START,
    OP_FUNCTION_END,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_NEW,
    OP_SET_MEMBER,
    OP_GET_MEMBER,
    OP_CALL_MEMBER,
    OP_CREATE_OBJECT,
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
    int ins_count;
    int ins_size;
} Chunk;

typedef struct ApexVM {
    CallFrame call_stack[CALL_STACK_MAX];
    int call_stack_top;
    bool in_function;
    Chunk *chunk;
    Ins *ins;
    ApexValue stack[STACK_MAX];
    ApexValue obj_context;
    int stack_top;
    int ip;
    int loop_start;
    int loop_end;
    SrcLoc srcloc;
    SymbolTable global_table;
    ScopeStack local_scopes;
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
extern void print_vm_instructions(ApexVM *vm);
extern void init_vm(ApexVM *vm);
extern void apexVM_reset(ApexVM *vm);
extern void free_vm(ApexVM *vm);
extern bool vm_dispatch(ApexVM *vm);

#endif
