#ifndef APEX_ERR_H
#define APEX_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include "apexVM.h"
#include "apexLex.h"

#define apexErr_fatal(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_mem(fmt, ...) do { \
    apexErr_error((SrcLoc){.lineno = 0, .filename = NULL}, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#define apexErr_syntax(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, fmt, ##__VA_ARGS__); \
} while (0)

#define apexErr_runtime(vm, fmt, ...) do { \
    apexErr_error(vm->ins->srcloc, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
} while (0)

extern void apexErr_error(SrcLoc srcloc, const char *fmt, ...);
extern void apexErr_trace(ApexVM *vm);

#endif