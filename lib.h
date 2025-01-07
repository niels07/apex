#ifndef LIB_H
#define LIB_H

#include "vm.h"

typedef void (*ApexLibFn)(ApexVM *vm);

typedef struct {
    char *name;
    ApexLibFn fn;
} ApexLibFnEntry;

typedef struct ApexLib {
    char *name;
    ApexLibFnEntry *fn;   
    int n; 
} ApexLib;

#define apex_regfn(name, fn) { name, fn }

#define apex_reglib(libname, ...)                   \
    void apex_register_##libname(void) {            \
        static ApexLibFnEntry entries[] = {         \
            __VA_ARGS__,                            \
            {NULL, NULL}                            \
        };                                          \
        for (int i = 0; entries[i].name != NULL; i++) {             \
            apexLib_add(#libname, entries[i].name, entries[i].fn);  \
        }                                                           \
    }

extern void apexLib_add(const char *libname, const char *fnname, ApexLibFn fn);
extern ApexLibFn apexLib_get(const char *libname, const char *fnname);
extern void apexLib_init(void);

#endif