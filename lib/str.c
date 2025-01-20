#include <string.h>
#include <regex.h> 
#include <ctype.h>
#include "apexVM.h"
#include "apexMem.h"
#include "apexVal.h"
#include "apexLib.h"
#include "apexErr.h"

int str_split(ApexVM *vm, int argc) {
    if (argc == 0 || argc > 2) {
        apexErr_runtime(vm, "function 'str:split' expects 1 or 2 arguments");
        return 1;
    }
    ApexValue strval = apexVM_pop(vm);

    if (apexVal_type(strval) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(strval));
        return 1;
    }

    char *delim;
    if (argc == 2) {
        ApexValue delimval = apexVM_pop(vm);

        if (apexVal_type(delimval) != APEX_VAL_STR) {
            apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(delimval));
            return 1;
        }

        delim = apexVal_str(delimval)->value;
    } else {
        delim = " ";
    }

    char *str = apexVal_str(strval)->value;
    ApexArray *result = apexVal_newarray();
    size_t idx  = 0;
    char *token = strtok(str, delim);
    while (token) {
        ApexString *tokenstr = apexStr_new(token, strlen(token));
        apexVal_arrayset(result, apexVal_makeint(idx++), apexVal_makestr(tokenstr));
        token = strtok(0, delim);
    }
    apexVM_pusharr(vm, result);
    return 0;
}

int str_match(ApexVM *vm, int argc) {
    // Retrieve arguments from the stack
    ApexValue patternval = apexVM_pop(vm);
    ApexValue strval = apexVM_pop(vm);

    // Validate argument types
    if (apexVal_type(strval) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(strval));
        return 1;
    }
    if (apexVal_type(patternval) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(patternval));
        return 1;
    }

    const char *str = apexVal_str(strval)->value;
    const char *pattern = apexVal_str(patternval)->value;

    // Initialize regex
    regex_t regex;
    int ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret != 0) {
        char error_msg[128];
        regerror(ret, &regex, error_msg, sizeof(error_msg));
        apexErr_runtime(vm, "invalid regex pattern: %s", error_msg);
        regfree(&regex);
        return 1;
    }

    // Create result array
    ApexArray *result = apexVal_newarray();
    size_t idx = 0;

    // Perform regex matching
    regmatch_t match[1];
    const char *search_start = str;

    while (regexec(&regex, search_start, 1, match, 0) == 0) {
        size_t match_start = match[0].rm_so;
        size_t match_end = match[0].rm_eo;
        size_t match_len = match_end - match_start;

        if (match_len == 0) {
            // Prevent infinite loops for zero-length matches
            search_start++;
            continue;
        }

        // Add to result array
        ApexString *match_str = apexStr_new(search_start + match_start, match_len);
        apexVal_arrayset(result, apexVal_makeint(idx++), apexVal_makestr(match_str));

        // Move search_start to after the last match
        search_start += match_end;
    }

    regfree(&regex);

    // Push result array onto the VM stack
    apexVM_pusharr(vm, result);
    return 0;
}

int str_sub(ApexVM *vm, int argc) {
    // Retrieve arguments from the stack
    ApexValue repl_val = apexVM_pop(vm);
    ApexValue ptrn_val = apexVM_pop(vm);
    ApexValue str_val = apexVM_pop(vm);

    // Validate argument types
    if (apexVal_type(str_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(str_val));
        return 1;
    }

    if (apexVal_type(ptrn_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(ptrn_val));
        return 1;
    }

    if (apexVal_type(repl_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s is not a string", apexVal_typestr(repl_val));
        return 1;
    }

    // Extract C strings from ApexValue arguments
    const char *str = apexVal_str(str_val)->value;
    const char *ptrn = apexVal_str(ptrn_val)->value;
    const char *repl = apexVal_str(repl_val)->value;

    // Compile the regex pattern
    regex_t regex;
    if (regcomp(&regex, ptrn, REG_EXTENDED) != 0) {
        apexErr_runtime(vm, "invalid regex pattern: '%s'", ptrn);
        return 1;
    }

    size_t result_size = strlen(str) * 2;
    size_t result_len = 0;
    char *result = apexMem_alloc(result_size);
    

    // Traverse the string and replace matches
    const char *cursor = str;
    regmatch_t match;

    while (regexec(&regex, cursor, 1, &match, 0) == 0) {
        // Copy the text before the match
        size_t prefix_len = match.rm_so;
        if (result_len + prefix_len >= result_size) {
            result_size *= 2;
            result = apexMem_realloc(result, result_size);
        }
        strncpy(result + result_len, cursor, prefix_len);
        result_len += prefix_len;

        // Copy the replacement
        size_t repl_len = strlen(repl);
        if (result_len + repl_len >= result_size) {
            result_size *= 2;
            result = apexMem_realloc(result, result_size);
        }
        strncpy(result + result_len, repl, repl_len);
        result_len += repl_len;

        // Move the cursor forward
        cursor += match.rm_eo;
    }

    // Append the remaining part of the string
    size_t remaining_len = strlen(cursor);
    if (result_len + remaining_len >= result_size) {
        result_size += remaining_len;
        result = apexMem_realloc(result, result_size);
    }
    strcpy(result + result_len, cursor);
    result_len += remaining_len;

    // Clean up regex resources
    regfree(&regex);

    // Create the result string and push it onto the stack
    ApexString *result_str = apexStr_save(result, result_len);
    apexVM_pushstr(vm, result_str);

    return 0;
}

int str_format(ApexVM *vm, int argc) {
    if (argc < 1) {
        apexErr_runtime(vm, "'str:format' expects at least 1 argument");
        return 1;
    }
    ApexValue args[argc];
    // Retrieve the format string
    for (int i = argc - 1; i >= 0; i--) {
        args[i] = apexVM_pop(vm);
    }

    if (apexVal_type(args[0]) != APEX_VAL_STR) {
        apexErr_runtime(vm, "first argument to 'str:format' must be a string");
        return 1;
    }

    const char *format = apexVal_str(args[0])->value;

    // Dynamic buffer for the formatted string
    size_t buffer_size = 128;
    size_t buffer_len = 0;
    char *result = apexMem_alloc(buffer_size);

    const char *cursor = format;
    int arg_index = 1;

    while (*cursor) {
        if (*cursor != '%') {
            if (buffer_len + 1 >= buffer_size) {
                buffer_size *= 2;
                result = apexMem_realloc(result, buffer_size);
            }
            result[buffer_len++] = *cursor++;
            continue;
        }
        cursor++; // Skip '%'

        if (*cursor == '\0') {
            free(result);
            apexErr_runtime(vm, "incomplete format specifier in 'str:format'");
            return 1;
        }

        char specifier = *cursor++;
        if (arg_index > argc - 1) {
            free(result);
            apexErr_runtime(vm, "not enough arguments for format string");
            return 1;
        }

        ApexValue arg = args[arg_index++];
        char buffer[256];
        size_t formatted_len = 0;

        switch (specifier) {
        case 's': // String
            if (apexVal_type(arg) != APEX_VAL_STR) {
                free(result);
                apexErr_runtime(vm, "expected string for format specifier %%s: %s", apexVal_tostr(arg)->value);
                return 1;
            }
            snprintf(buffer, sizeof(buffer), "%s", apexVal_str(arg)->value);
            break;

        case 'd': // Integer
            if (apexVal_type(arg) != APEX_VAL_INT) {
                free(result);
                apexErr_runtime(vm, "expected integer for format specifier %%d");
                return 1;
            }
            snprintf(buffer, sizeof(buffer), "%d", arg.intval);
            break;

        case 'f': // Float or Double
            if (apexVal_type(arg) == APEX_VAL_FLT) {
                snprintf(buffer, sizeof(buffer), "%f", arg.fltval);
            } else if (apexVal_type(arg) == APEX_VAL_DBL) {
                snprintf(buffer, sizeof(buffer), "%lf", arg.dblval);
            } else {
                free(result);
                apexErr_runtime(vm, "expected float or double for format specifier %%f");
                return 1;
            }
            break;

        case '%': // Literal '%'
            snprintf(buffer, sizeof(buffer), "%%");
            break;

        default:
            free(result);
            apexErr_runtime(vm, "unknown format specifier '%c'", specifier);
            return 1;
        }

        formatted_len = strlen(buffer);

        // Ensure the result buffer can accommodate the new data
        if (buffer_len + formatted_len + 1 > buffer_size) {
            buffer_size = buffer_len + formatted_len + 1;
            result = apexMem_realloc(result, buffer_size);
        }

        // Append the formatted string to the result
        strcpy(result + buffer_len, buffer);
        buffer_len += formatted_len;

    }

    // Null-terminate the result
    result[buffer_len] = '\0';

    // Push the formatted string onto the stack
    ApexString *result_str = apexStr_save(result, buffer_len);
    apexVM_pushstr(vm, result_str);
    return 0;
}

int str_lower(ApexVM *vm, int argc) {
    ApexValue str_val = apexVM_pop(vm);
    if (apexVal_type(str_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument to 'str:lower' must be a string");
        return 1;
    }

    ApexString *input_str = apexVal_str(str_val);
    size_t len = input_str->len;

    // Allocate memory for the result string
    char *lower_str = apexMem_alloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        lower_str[i] = tolower(input_str->value[i]);
    }
    lower_str[len] = '\0';

    ApexString *result_str = apexStr_save(lower_str, len);
    apexVM_pushstr(vm, result_str);
    return 0;
}

int str_upper(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "function 'str:upper' expects exactly 1 argument");
        return 1;
    }

    ApexValue str_val = apexVM_pop(vm);
    if (apexVal_type(str_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument to 'str:upper' must be a string");
        return 1;
    }

    ApexString *input_str = apexVal_str(str_val);
    size_t len = input_str->len;

    // Allocate memory for the result string
    char *upper_str = apexMem_alloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        upper_str[i] = toupper(input_str->value[i]);
    }
    upper_str[len] = '\0';

    ApexString *result_str = apexStr_save(upper_str, len);
    apexVM_pushstr(vm, result_str);
    return 0;
}

apex_reglib(str,
    apex_regfn("split", str_split),
    apex_regfn("match", str_match),
    apex_regfn("sub", str_sub),
    apex_regfn("format", str_format),
    apex_regfn("upper", str_upper),
    apex_regfn("lower", str_lower)
);