#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "vm.h"
#include "parser.h"


/**
 * Prints an error message to stderr with a newline at the end.
 * 
 * The message is formatted according to the given format string and
 * additional arguments, and is prepended with "Error: ".
 *
 * @param fmt The format string of the error message.
 * @param ... The arguments to the format string.
 */
void apexErr_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr,"\n");
    va_end(args);
}

/**
 * Prints a fatal error message to stderr and terminates the program.
 * 
 * This function formats an error message according to the given format string
 * and additional arguments, prepends "Error: " to the message, and appends the
 * line number and filename from the provided ParseState. The message is 
 * printed to stderr with a newline at the end, and the program is then 
 * terminated with an EXIT_FAILURE status.
 *
 * @param state The ParseState providing the line number and filename.
 * @param fmt The format string of the error message.
 * @param ... The arguments to the format string.
 */
void apexErr_fatal(SrcLoc srcloc, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr," on line %d in %s\n", srcloc.lineno, srcloc.filename);
    va_end(args);
    exit(EXIT_FAILURE);
}


/**
 * Prints a syntax error message to stderr with a newline at the end. The
 * message is formatted according to fmt and the variable arguments following
 * it. The line number and filename are also included in the message.
 *
 * @param state The ParseState providing the line number and filename.
 * @param fmt The format string of the error message.
 * @param ... The arguments to the format string.
 */
void apexErr_syntax(SrcLoc srcloc, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Syntax Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr," on line %d in %s\n", srcloc.lineno, srcloc.filename);
    va_end(args);
}

/**
 * Prints a runtime error message to stderr with the specified line number.
 * The message is formatted according to fmt and the variable arguments following it.
 * A newline is appended at the end of the message.
 * Exits the program with a status of EXIT_FAILURE after printing the error message.
 */
void apexErr_runtime(ApexVM *vm, const char *fmt, ...) {
    va_list args;
    int i;

    fprintf(
        stderr, "Runtime Error (line %d, file %s): ", 
        vm->ins->srcloc.lineno, 
        vm->ins->srcloc.filename);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\nStack trace:\n");

    for (i = vm->call_stack_top - 1; i >= 0; i--) {
        CallFrame *frame = &vm->call_stack[i];
        fprintf(
            stderr, "  at %s (line %d) in %s\n",
            frame->fn_name ? frame->fn_name : "<main>",
            frame->srcloc.lineno,
            i == 0 ? "<main>" : frame->fn_name);
    }

    exit(EXIT_FAILURE);
}