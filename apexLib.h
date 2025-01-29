#ifndef APEX_LIB_H
#define APEX_LIB_H

#include "apexVM.h"

/**
 * Represents a function in a library, with its name and a pointer to the function.
 */
typedef struct {
    char *name; /** The name of the data. */
    bool is_var; /** Whether the entry is a variable. */
    union {
        int (*fn)(ApexVM *, int); /** Pointer to the function implementation. */
        ApexValue *var; /** Pointer to the variable member. */
    };
} ApexLibData;

/**
 * Represents a library containing a function.
 */
typedef struct ApexLib {
    char *name; /** The name of the library. */
    ApexLibData data; /** The function contained in the library. */
} ApexLib;

/**
 * Macro to define a library function entry.
 * 
 * @param name The name of the function.
 * @param fn The function implementation.
 */
#define apex_regfn(NAME, FN) { NAME, false, { .fn = FN } }

/**
 * Macro to define a library variable entry.
 *
 * @param name The name of the variable.
 * @param var The variable value.
 */
#define apex_regvar(NAME, VAR) { NAME, true, { .var = &VAR } }

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
        static ApexLibData entries[] = {           \
            __VA_ARGS__,                            \
            {NULL, false, NULL}                     \
        };                                          \
        for (int i = 0; entries[i].name != NULL; i++) {             \
            apexLib_add(#libname, entries[i].name, entries[i]);  \
        }                                                           \
    }

extern void apexLib_add(const char *libname, const char *fnname, ApexLibData data);
extern ApexLibData apexLib_get(const char *libname, const char *fnname);
extern void apexLib_init(void);
extern void apexLib_free(void);

#endif