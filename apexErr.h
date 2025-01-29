#ifndef APEX_ERR_H
#define APEX_ERR_H

#include <stdio.h>
#include <stdlib.h>
#include "apexVM.h"
#include "apexLex.h"

/**
 * Fatal error macro.
 *
 * This macro will print an error message to stderr and
 * exit the program with a non-zero status code.
 *
 * @param srcloc The source location of the error.
 * @param fmt The format string for the error message.
 * @param ... The arguments for the format string.
 */
#define apexErr_fatal(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

/**
 * Out-of-memory error macro.
 *
 * This macro will print an error message to stderr and
 * exit the program with a non-zero status code.
 *
 * @param fmt The format string for the error message.
 * @param ... The arguments for the format string.
 */
#define apexErr_mem(fmt, ...) do { \
    apexErr_error((SrcLoc){.lineno = 0, .filename = NULL}, fmt, ##__VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

/**
 * Syntax error macro.
 *
 * This macro will print an error message to stderr.
 *
 * @param srcloc The source location of the error.
 * @param fmt The format string for the error message.
 * @param ... The arguments for the format string.
 */
#define apexErr_syntax(srcloc, fmt, ...) do { \
    apexErr_error(srcloc, fmt, ##__VA_ARGS__); \
} while (0)

/**
 * Runtime error macro.
 *
 * This macro will print an error message to stderr and
 * print a stack trace of the current call stack.
 *
 * @param vm The VM containing the current call stack.
 * @param fmt The format string for the error message.
 * @param ... The arguments for the format string.
 */
#define apexErr_runtime(vm, fmt, ...) do { \
    apexErr_error(vm->ins->srcloc, fmt, ##__VA_ARGS__); \
    apexErr_trace(vm); \
} while (0)

extern void apexErr_error(SrcLoc srcloc, const char *fmt, ...);
extern void apexErr_trace(ApexVM *vm);

#endif