#include <string.h>
#include <regex.h> 
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

apex_reglib(str,
    apex_regfn("split", str_split, -1),
    apex_regfn("match", str_match, 2)
);