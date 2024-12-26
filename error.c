#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * Prints an error message to stderr with a newline at the end. The message is
 * formatted according to fmt and the variable arguments following it.
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
 * Prints an error message to stderr with a newline at the end, and then exits
 * the program with a status of EXIT_FAILURE. The message is formatted according
 * to fmt and the variable arguments following it.
 */
void apexErr_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr,"\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

/**
 * Prints a syntax error message to stderr with the specified line number.
 * The message is formatted according to fmt and the variable arguments following it.
 * A newline is appended at the end of the message.
 */
void apexErr_syntax(int lineno, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Syntax Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr," on line %d\n", lineno);
    va_end(args);
}

/**
 * Prints a runtime error message to stderr with the specified line number.
 * The message is formatted according to fmt and the variable arguments following it.
 * A newline is appended at the end of the message.
 * Exits the program with a status of EXIT_FAILURE after printing the error message.
 */
void apexErr_runtime(int lineno, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Runtime Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr," on line %d\n", lineno);
    va_end(args);
    exit(EXIT_FAILURE);
}