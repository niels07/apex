#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "srcloc.h"

typedef enum {
    E_WARN,
    E_FATAL,
    E_SYNTAX,
    E_RUNTIME,
    E_TYPE, 
    E_MEM,
    E_NAME,
    E_ARG,
} ErrorType;

#define apexErr_warn(srcloc, fmt, ...) \
    apexErr_error(srcloc, E_WARN, fmt, ##__VA_ARGS__); 

#define apexErr_fatal(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, E_FATAL, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_mem(fmt, ...) do { \
    apexErr_error((SrcLoc){.lineno = 0, .filename = NULL}, E_MEM, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_syntax(lexer, fmt, ...) do { \
    apexErr_error(lexer->srcloc, E_SYNTAX, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_name(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, E_NAME, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_arg(vm, fmt, ...) do { \
    apexErr_error(vm->ins->srcloc, E_ARG, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_runtime(vm, fmt, ...) do { \
    apexErr_error(vm->ins->srcloc, E_RUNTIME, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_type(vm, fmt, ...) do { \
    apexErr_error(vm->ins->srcloc, E_TYPE, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
    exit(EXIT_FAILURE); \
} while (0)

extern void apexErr_error(SrcLoc srcloc, ErrorType type, const char *fmt, ...);
extern void apexErr_trace(ApexVM *vm);

#endif