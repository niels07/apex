#ifndef STDLIB_H
#define STDLIB_H

#include <stdio.h>
#include "vm.h"

typedef void (*ApexFn)(ApexVM *vm);

typedef struct {
    char *name;
    ApexFn fn;
} StdLib;

extern void apex_write(ApexVM *vm);
extern void apex_print(ApexVM *vm);
extern void apex_read(ApexVM *vm);


extern StdLib apex_stdlib[];

#endif