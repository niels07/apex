#ifndef APEX_API_H
#define APEX_API_H

#include "vm.h"

typedef void (*ApexApi_function)(ApexVm *vm, int);
typedef struct ApexApi_entry {
    char name[64];
    ApexApi_function func;
} ApexApi_entry; 

extern void ApexApi_register(ApexVm *, const char *, ApexApi_function);

#endif
