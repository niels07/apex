#ifndef APX_VM_H
#define APX_VM_H

#include "stddef.h"
#include "hash.h"

typedef struct ApexVm ApexVm;

typedef unsigned char ApexByte; 

typedef enum {
    APEX_VM_OP_PUSH_INT,
    APEX_VM_OP_ADD,
    APEX_VM_OP_SUB,
    APEX_VM_OP_MUL,
    APEX_VM_OP_DIV,
    APEX_VM_OP_MOD,
    APEX_VM_OP_AND,
    APEX_VM_OP_PRINT,
    APEX_VM_OP_END
} ApexVmOp;

typedef struct ApexVmCode {
    ApexByte *bytes;
    size_t size;
    size_t n;
} ApexVmCode;

extern ApexVm *apex_vm_new(ApexHashTable *, const char *);
extern ApexVm *apex_vm_new_debug(const char *);
extern void apex_vm_dispatch(ApexVm *);
extern void apex_vm_print(ApexVm *);
extern void apex_vm_set_debug(ApexVm *, int);
extern void apex_vm_destroy(ApexVm *);

extern ApexValue *apex_vm_pop(ApexVm *);
extern ApexValue *apex_vm_top(ApexVm *);

#endif
