#ifndef APX_VM_H
#define APX_VM_H

#include <stdio.h>
#include "hash.h"
#include "value.h"
#include "api.h"

#ifndef APEX_VM_DEFINED
#define APEX_VM_DEFINED
    typedef struct ApexVm ApexVm;
#endif

#ifndef APEX_VALUE_DEFINED
#define APEX_VALUE_DEFINED
    typedef struct ApexValue ApexValue;
#endif

#ifndef APEX_HASH_TABLE_DEFINED
#define APEX_HASH_TABLE_DEFINED
    typedef struct ApexHash_table ApexHash_table;
#endif

typedef unsigned char ApexByte;

typedef enum {
    APEX_VM_OP_PUSH_INT,
    APEX_VM_OP_PUSH_FLT,
    APEX_VM_OP_PUSH_STR,
    APEX_VM_OP_ADD,
    APEX_VM_OP_SUB,
    APEX_VM_OP_MUL,
    APEX_VM_OP_DIV,
    APEX_VM_OP_EQUALS,
    APEX_VM_OP_LOAD_REF,
    APEX_VM_OP_STORE_REF,
    APEX_VM_OP_CALL,
    APEX_VM_OP_IF,
    APEX_VM_OP_ELSE,
    APEX_VM_OP_NOT,
    APEX_VM_OP_WHILE,
    APEX_VM_OP_FUNC_DEF,
    APEX_VM_OP_END
} ApexVmOp;

#define MAX_API_ENTRIES 100

typedef struct ApexVmCode {
    ApexByte *bytes;
    size_t size;
    size_t n;
} ApexVmCode;

struct ApexVm {
    ApexVmCode code;
    ApexByte *p;
    ApexValue *stack;
    ApexValue *stack_top;
    size_t stack_size;
    size_t stack_n;
    ApexHash_table *table;
    ApexApi_entry api_entries[MAX_API_ENTRIES];
    int api_entry_count;
};

ApexVm *ApexVm_new(ApexHash_table *, const char *);
ApexVm *ApexVm_new_debug(const char *);
void ApexVm_dispatch(ApexVm *);
ApexValue *ApexVm_top(ApexVm *);
void ApexVm_destroy(ApexVm *);
ApexValue *ApexVm_pop(ApexVm *);
ApexValue *ApexVm_top(ApexVm *);

#endif



