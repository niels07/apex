#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include "mem.h"
#include "string.h"

/**
 * Converts a string to an integer.
 *
 * This function attempts to parse the input string as a signed integer. It
 * supports optional leading '+' or '-' signs. If the conversion is successful,
 * the resulting integer value is stored in the location pointed to by 'out',
 * and the function returns true. If the string is invalid or cannot be
 * converted, the function returns false.
 *
 * @param out A pointer to an int where the result will be stored if conversion
 *            is successful.
 * @param str The input string to be converted to an int.
 * @return true if the conversion is successful, false otherwise.
 */
bool apexUtl_stoi(int *out, const char *str) {
     if (!str || !*str) {
        return false;
    }

    char *endptr;
    errno = 0;

    // Convert the string to int
    long value = strtol(str, &endptr, 10); // Base 10 for integer conversion

    if (errno == ERANGE || value > INT_MAX || value < INT_MIN) {
        // Overflow or underflow occurred
        return false;
    }
    if (endptr == str) {
        // No digits were found
        return false;
    }
    if (*endptr != '\0') {
        // Additional invalid characters after number
        return false;
    }

    *out = (int)value;
    return true;
}

/**
 * Converts a string to a floating-point number.
 *
 * This function attempts to parse the input string as a floating-point number.
 * It supports optional leading '+' or '-' signs and decimal points. Exponential
 * notation with 'e' or 'E' is also supported. If the conversion is successful,
 * the resulting float value is stored in the location pointed to by 'out', and
 * the function returns true. If the string is invalid or cannot be converted,
 * the function returns false.
 *
 * @param out A pointer to a float where the result will be stored if conversion is successful.
 * @param str The input string to be converted to a float.
 * @return true if the conversion is successful, false otherwise.
 */
bool apexUtl_stof(float *out, const char *str) {
    if (!str || !*str) {
        return false;
    }
    char *endptr;
    errno = 0;
    float value = strtof(str, &endptr);
    if (errno == ERANGE) {
        // Overflow or underflow occurred
        return false;
    }
    if (str == endptr) {
        // No digits were found
        return false;
    }
    if (*endptr != '\0') {
        // Additional invalid characters after number
        return false;
    }
    *out = value;
    return true;
}

/**
 * Converts a string to a double.
 *
 * This function attempts to parse the input string as a double. It supports
 * optional leading '+' or '-' signs and decimal points. Exponential notation
 * with 'e' or 'E' is also supported. If the conversion is successful, the
 * resulting double value is stored in the location pointed to by 'out', and the
 * function returns true. If the string is invalid or cannot be converted, the
 * function returns false.
 *
 * @param out A pointer to a double where the result will be stored if conversion is successful.
 * @param str The input string to be converted to a double.
 * @return true if the conversion is successful, false otherwise.
 */
bool apexUtl_stod(double *out, const char *str) {
    if (!str || !*str) {
        return false;
    }
    char *endptr;
    errno = 0;
    double value = strtod(str, &endptr);
    if (errno == ERANGE) {
        // Overflow or underflow occurred
        return false;
    }
    if (str == endptr) {
        // No digits were found
        return false;
    }
    if (*endptr != '\0') {
        // Additional invalid characters after number
        return false;
    }
    *out = value;
    return true;
}

/**
 * Reads a line of text from the given stream and returns a pointer to it.
 *
 * This function dynamically allocates memory to store the line read from
 * the input stream. It continues reading characters until a newline or EOF
 * is encountered. If the end of the file is reached without reading any
 * characters, the function returns NULL. Otherwise, it returns a pointer
 * to the saved line in the string table. Memory is reallocated as needed
 * to accommodate longer lines.
 *
 * @param stream The file stream to read from.
 * @return A pointer to the line of text that was read, or NULL if end of
 *         file was encountered without reading any characters.
 */
ApexString *apexUtl_readline(FILE *stream) {
    size_t size = 128;
    size_t len = 0;
    char *buffer = apexMem_alloc(size);
    int c;

    while ((c = getc(stream)) != '\n' && c != EOF) {
        if (len + 1 >= size) {
            size *= 2;
            buffer = apexMem_realloc(buffer, size);
        }
        buffer[len++] = c;
    }

    if (len == 0 && c == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';

    return apexStr_save(buffer, len);
}