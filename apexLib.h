#ifndef APEX_LIB_H
#define APEX_LIB_H

#include "apexVM.h"

typedef struct {
    char *name;
    int (*fn)(ApexVM *, int);
} ApexLibFn;

typedef struct ApexLib {
    char *name;
    ApexLibFn fn;   
} ApexLib;

#define apex_regfn(name, fn ) { name, fn }

/**
 * Registers a library with the Apex runtime.
 *
 * This macro defines a `void apex_register_<libname>(void)` function that
 * registers all the functions in the library with the Apex runtime. The
 * library name and the functions to be registered are specified as
 * arguments to the macro.
 *
 * @param libname The name of the library.
 * @param ... A list of `apex_regfn` entries, each specifying the name of
 *            a function and the associated function pointer.
 */
#define apex_reglib(libname, ...)                   \
    void apex_register_##libname(void) {            \
        static ApexLibFn entries[] = {              \
            __VA_ARGS__,                            \
            {NULL, NULL}                            \
        };                                          \
        for (int i = 0; entries[i].name != NULL; i++) {             \
            apexLib_add(#libname, entries[i].name, entries[i]);  \
        }                                                           \
    }

extern void apexLib_add(const char *libname, const char *fnname, ApexLibFn fn);
extern ApexLibFn apexLib_get(const char *libname, const char *fnname);
extern void apexLib_init(void);
extern void apexLib_free(void);

#endif