#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "vm.h"
#include "parser.h"
#include "error.h"

/**
 * Prints an error message to stderr with the given error type and format
 * string.
 *
 * The error message will be prefixed with "Fatal Error: ", "Syntax Error: ",
 * "Runtime Error: ", "Type Error: ", or "Memory Error: " depending on the
 * given error type.
 *
 * If the given ParseState has a non-zero line number, the error message will
 * include the line number and filename. Otherwise, only a colon will be
 * printed.
 *
 * The given format string and arguments will be used to generate the error
 * message.
 *
 * If errno is non-zero, the error message will include the error string for
 * the current errno.
 */
void apexErr_error(ParseState parsestate, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "error");
    if (parsestate.lineno && parsestate.filename) {
        fprintf(stderr,"(line %d, file %s): ", 
        parsestate.lineno, 
        parsestate.filename);
    } else {
        fprintf(stderr, ": ");
    }
    vfprintf(stderr, fmt, args);
    if (errno) {
        fprintf(stderr, ": %s", strerror(errno));
    }
    fprintf(stderr,"\n");
    va_end(args);
}

/**
 * Prints a stack trace to stderr of the given virtual machine.
 *
 * The stack trace will consist of one line per call frame, with the line
 * number and filename of the call site. If the call site is the "main" fn,
 * the filename will be "<main>" instead of the actual filename.
 *
 * @param vm The virtual machine to print the stack trace of.
 */
void apexErr_trace(ApexVM *vm) {
    if (!vm->call_stack_top) {
        return;
    }
    fprintf(stderr, "Stack trace:\n");

    for (int i = vm->call_stack_top - 1; i >= 0; i--) {
        CallFrame *frame = &vm->call_stack[i];
        fprintf(
            stderr, "  at %s (line %d) in %s\n",
            frame->fn_name ? frame->fn_name : "<main>",
            frame->parsestate.lineno,
            i == 0 ? "<main>" : frame->fn_name);
    }
}