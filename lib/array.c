#include "string.h"
#include "apexVM.h"
#include "apexVal.h"
#include "apexErr.h"
#include "apexMem.h"
#include "apexLib.h"

/**
 * Checks if a given key exists in an array.
 *
 * This function expects two arguments: an array and a key. It checks whether
 * the specified key is present in the provided array and pushes a boolean
 * result onto the virtual machine's stack indicating the presence of the key.
 *
 * If the number of arguments is not exactly two, or if the key is not a string,
 * or if the first argument is not an array, a runtime error is raised.
 *
 * @param vm A pointer to the virtual machine.
 * @param argc The number of arguments passed to the function.
 * @return Returns 0 on success, or 1 if an error occurs.
 */
int array_key_exists(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "array:key_exists expects exactly 2 arguments");
        return 1;
    }
    ApexValue key_val = apexVM_pop(vm);

    if (apexVal_type(key_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument %s must be a string", apexVal_typestr(key_val));
        return 1;
    }

    ApexValue array_val = apexVM_pop(vm);
    if (apexVal_type(array_val) != APEX_VAL_ARR) {
        apexErr_runtime(vm, "argument %s must be an array", apexVal_typestr(array_val));
        return 1;
    }

    ApexValue value;
    bool key_exists = apexVal_arrayget(&value, array_val.arrval, key_val);
    apexVM_pushbool(vm, key_exists);
    return 0;
}

/**
 * Joins an array of strings into a single string, with an optional delimiter
 * separating the strings.
 *
 * The first argument to this function is an array of strings. The second
 * argument is an optional delimiter string. If the second argument is not
 * provided, the strings are joined without a delimiter.
 *
 * @param array The array of strings to join.
 * @param delim An optional delimiter to separate the strings with.
 * @return The joined string.
 */
int array_join(ApexVM *vm, int argc) {
    if (argc > 2) {
        apexErr_runtime(vm, "array:join expects at most 2 arguments");
        return 1;
    }
    char *delim = NULL;
    if (argc == 2) {
        ApexValue delim_val = apexVM_pop(vm);
        if (apexVal_type(delim_val) != APEX_VAL_STR) {
            apexErr_runtime(vm, "second argument to array:join must be a string");
            return 1;
        }
        delim = apexVal_str(delim_val)->value;
    }
    ApexValue array_val = apexVM_pop(vm);
    if (apexVal_type(array_val) != APEX_VAL_ARR) {
        apexErr_runtime(vm, "first argument to array:join must be an array");
        return 1;
    }
    ApexArray *array = apexVal_array(array_val);
    size_t delim_len = delim ? strlen(delim) : 0;

    size_t joined_size = 256;
    size_t joined_len = 0;
    char *joined = apexMem_alloc(joined_size);
    *joined = '\0';
    bool first = true;    

    apexArray_each(array) {
        ApexArrayEntry *entry = apexArray_next(array);
        char *value = apexVal_tostr(entry->value)->value;
        size_t len = strlen(value);
        
        joined_len += len + (first ? 0 : delim_len);
        if (joined_len >= joined_size) {
            while (joined_len >= joined_size) {
                joined_len *= 2;
            }
            joined = apexMem_realloc(joined, joined_size);
        }
        if (!first && delim) {
            strcat(joined, delim);
        }
        strcat(joined, value);
        first = false;        
    }
    ApexString *joined_str = apexStr_save(joined, joined_len);
    apexVM_pushstr(vm, joined_str);
    return 0;
}

/**
 * Applies a given function to each element in an array and returns a new array
 * containing the results.
 *
 * This function expects two arguments: an array and a function. It iterates over
 * each element in the provided array, applies the function to the element, and
 * collects the results in a new array. The function must accept a single argument
 * and return a value that will be included in the resulting array.
 *
 * If the number of arguments is not exactly two, or if the provided function is not
 * of type APEX_VAL_FN, or if the first argument is not an array, a runtime error
 * is raised.
 *
 * @param vm A pointer to the virtual machine.
 * @param argc The number of arguments passed to the function.
 * @return Returns 0 on success, or 1 if an error occurs.
 */
int array_map(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "array:map expects exactly 2 arguments");
        return 1;
    }

    ApexValue fn_val = apexVM_pop(vm);
    if (apexVal_type(fn_val) != APEX_VAL_FN) {
        apexErr_runtime(vm, "second argument to array:map must be a function");
        return 1;
    }

    ApexValue array_val = apexVM_pop(vm);
    if (apexVal_type(array_val) != APEX_VAL_ARR) {
        apexErr_runtime(vm, "first argument to array:map must be an array");
        return 1;
    }

    ApexArray *array = apexVal_array(array_val);
    ApexArray *new_array = apexVal_newarray();
    int array_index = 0;
    apexArray_each(array) {
        ApexArrayEntry *entry = apexArray_next(array);
        apexVM_pushval(vm, entry->value);
        if (!apexVM_call(vm, apexVal_fn(fn_val), 1)) {
            return 1;
        }
        if (vm->stack_top > 0) {
            apexVal_arrayset(new_array, apexVal_makeint(array_index++), apexVM_pop(vm));
        }
    }
    apexVM_pusharr(vm, new_array);
    apexVal_release(fn_val);
    return 0;
}

apex_reglib(array,
    apex_regfn("key_exists", array_key_exists),
    apex_regfn("join", array_join),
    apex_regfn("map", array_map)
);