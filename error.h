#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "srcloc.h"

#define apexErr_fatal(parsestate, fmt, ...) do { \
    apexErr_error(parsestate, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_mem(fmt, ...) do { \
    apexErr_error((ParseState){.lineno = 0, .filename = NULL}, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_syntax(parsestate, fmt, ...) do { \
    apexErr_error(parsestate, fmt, ##__VA_ARGS__); \
} while (0)

#define apexErr_runtime(vm, fmt, ...) do { \
    apexErr_error(vm->ins->parsestate, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
} while (0)

extern void apexErr_error(ParseState parsestate, const char *fmt, ...);
extern void apexErr_trace(ApexVM *vm);

#endif