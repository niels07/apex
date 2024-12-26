#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include "bool.h"

/**
 * Converts a string to an integer.
 *
 * This function attempts to parse the input string as a signed integer. It
 * supports optional leading '+' or '-' signs. If the conversion is successful,
 * the resulting integer value is stored in the location pointed to by 'out',
 * and the function returns TRUE. If the string is invalid or cannot be
 * converted, the function returns FALSE.
 *
 * @param out A pointer to an int where the result will be stored if conversion
 *            is successful.
 * @param str The input string to be converted to an int.
 * @return TRUE if the conversion is successful, FALSE otherwise.
 */
Bool apexUtl_stoi(int *out, const char *str) {
    int result = 0;
    int sign = 1;

    if (!str || *str == '\0') {
        return FALSE;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str) {
        if (*str < '0' || *str > '9') {
            return FALSE;
        }

        if (result > (INT_MAX - (*str - '0')) / 10) {
            return FALSE;
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    *out = result * sign;
    return TRUE;
}

/**
 * Converts a string to a floating-point number.
 *
 * This function attempts to parse the input string as a floating-point number.
 * It supports optional leading '+' or '-' signs and decimal points. Exponential
 * notation with 'e' or 'E' is also supported. If the conversion is successful,
 * the resulting float value is stored in the location pointed to by 'out', and
 * the function returns TRUE. If the string is invalid or cannot be converted,
 * the function returns FALSE.
 *
 * @param out A pointer to a float where the result will be stored if conversion is successful.
 * @param str The input string to be converted to a float.
 * @return TRUE if the conversion is successful, FALSE otherwise.
 */
Bool apexUtl_stof(float *out, const char *str) {
    float result = 0.0f;
    int sign = 1;
    Bool has_decimal = FALSE;
    float decimal_factor = 0.1f;

    if (str == NULL || *str == '\0') {
        return FALSE;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    while (*str) {
        if (*str == '.') {
            if (has_decimal) {
                return FALSE;
            }
            has_decimal = TRUE;
            str++;
            continue;
        }

        if (*str >= '0' && *str <= '9') {
            if (has_decimal) {
                result += (*str - '0') * decimal_factor;
                decimal_factor *= 0.1f;
            } else {
                if (result > (FLT_MAX - (*str - '0')) / 10) {
                    return FALSE;
                }
                result = result * 10 + (*str - '0');
            }
        } else {
            break;
        }
        str++;
    }

    if (*str == 'e' || *str == 'E') {
        int exp_sign = 1;
        int exponent = 0;

        str++;
        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        if (!isdigit(*str)) {
            return FALSE;
        }

        while (*str >= '0' && *str <= '9') {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        result *= pow(10.0f, exp_sign * exponent);
    }

    if (*str) {
        return FALSE;
    }

    *out = sign * result;
    return TRUE;
}

/**
 * A safer version of snprintf that will not write past the end of the given buffer.
 *
 * @param buffer The buffer to write to.
 * @param size The size of the buffer to write to.
 * @param format The format string to use.
 * @param ... The arguments to the format string.
 *
 * @return The number of characters that were written.
 */
int apexUtl_snprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args;
    int written;
    char temp[1024];

    va_start(args, format);    
    written = vsprintf(temp, format, args);

    if (size > 0) {
        size_t to_copy = (size - 1 < (size_t)written) ? size - 1 : (size_t)written;
        size_t i;
        for (i = 0; i < to_copy; i++) {
            buffer[i] = temp[i];
        }
        buffer[to_copy] = '\0';
    }

    va_end(args);
    return written;
}