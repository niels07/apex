
#include <stdlib.h>
#include <stdio.h>

#include "apexVM.h"
#include "apexVal.h"
#include "apexUtil.h"
#include "apexErr.h"
#include "apexLib.h"
#include "apexStr.h"

/**
 * std:int(value)
 *
 * Converts the given value to an integer.
 *
 * The function supports conversion from the following types:
 * - Boolean: Converts true to 1 and false to 0.
 * - Integer: Returns the integer value as is.
 * - Float: Converts the float to an integer by truncation.
 * - Double: Converts the double to an integer by truncation.
 * - String: Attempts to parse the string as an integer.
 *
 * If the conversion is not possible, a runtime error is raised.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param argc The number of arguments, expected to be 1.
 * @return Returns 0 on successful conversion, or 1 if an error occurs.
 */
int std_int(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:int expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_BOOL:
        apexVM_pushint(vm, apexVal_bool(value));
        break;

    case APEX_VAL_INT:
        apexVM_pushint(vm, apexVal_int(value));
        break;

    case APEX_VAL_FLT:
        apexVM_pushint(vm, (int)apexVal_flt(value));
        break;

    case APEX_VAL_DBL:
        apexVM_pushint(vm, (int)apexVal_dbl(value));
        break;

    case APEX_VAL_STR: {
        int i;
        if (apexUtil_stoi(&i, apexVal_str(value)->value)) {
            apexVM_pushint(vm, i);
        } else {
            apexErr_runtime(vm, "cannot convert string \"%s\" to int", apexVal_str(value)->value);
            return 1;
        }
        break;
    }

    default:
        apexErr_runtime(vm, "cannot convert %s to int", apexVal_typestr(value));
        return 1;
    }
    return 0;
}

/**
 * std:str(value)
 *
 * Converts the given value to a string.
 *
 * If the value cannot be converted to a string, a runtime error is raised.
 *
 * @param vm A pointer to the virtual machine.
 * @param argc The number of arguments passed to the function.
 * @return Returns 0 on success, or 1 if an error occurs.
 */
int std_str(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:str expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    ApexString *str = apexVal_tostr(value);
    apexVM_pushstr(vm, str);
    return 0;
}

/**
 * std:flt(value)
 *
 * Converts the given value to a float.
 *
 * The function supports conversion from the following types:
 * - Integer: Converts the integer to a float.
 * - Float: Returns the float value as is.
 * - Double: Converts the double to a float.
 * - String: Attempts to parse the string as a float.
 * - Boolean: Converts true to 1.0f and false to 0.0f.
 *
 * If the conversion is not possible, a runtime error is raised.
 *
 * @param vm A pointer to the virtual machine instance.
 * @param argc The number of arguments, expected to be 1.
 * @return Returns 0 on successful conversion, or 1 if an error occurs.
 */
int std_flt(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:flt expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        apexVM_pushflt(vm, (float)apexVal_int(value));
        break;
    case APEX_VAL_FLT:
        apexVM_pushval(vm, value);
        break;
    case APEX_VAL_DBL:
        apexVM_pushflt(vm, (float)apexVal_dbl(value));
        break;
    case APEX_VAL_STR: {
        float f;
        if (apexUtil_stof(&f, apexVal_str(value)->value)) {
            apexVM_pushflt(vm, f);
        } else {
            apexErr_runtime(vm, "cannot convert string \"%s\" to flt", apexVal_str(value)->value);
            return 1;
        }
        break;
    }
   case APEX_VAL_BOOL:
        apexVM_pushflt(vm, apexVal_bool(value));
        break;

    default:
        apexErr_runtime(vm, "cannot convert %s to flt", apexVal_typestr(value));
        return 1;
    }
    return 0;
}

/**
 * std:dbl(value)
 *
 * Converts the given value to a double.
 *
 * The following values are considered false:
 * - null
 * - false
 *
 * All other values are considered true.
 *
 * @param value The value to convert to a double.
 * @return The double value of the given argument.
 */
int std_dbl(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:dbl expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        apexVM_pushdbl(vm, (double)apexVal_int(value));
        break;
    case APEX_VAL_FLT:
        apexVM_pushdbl(vm, (double)apexVal_flt(value));
        break;
    case APEX_VAL_DBL:
        apexVM_pushval(vm, value);
        break;
    case APEX_VAL_STR: {
        double d;
        if (apexUtil_stod(&d, apexVal_str(value)->value)) {
            apexVM_pushdbl(vm, d);
        } else {
            apexErr_runtime(vm, "cannot convert string \"%s\" to dbl", apexVal_str(value)->value);
            return 1;
        }
        break;
    }
   case APEX_VAL_BOOL:
        apexVM_pushdbl(vm, apexVal_bool(value));
        break;
    default:
        apexErr_runtime(vm, "cannot convert %s to dbl", apexVal_typestr(value));
        return 1;
    }
    return 0;
}

/**
 * std:bool(value)
 *
 * Converts the given value to a boolean.
 *
 * The following values are considered false:
 * - null
 * - false
 *
 * All other values are considered true.
 *
 * @param value The value to convert to a boolean.
 * @return The boolean value of the given argument.
 */
int std_bool(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:bool expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
    case APEX_VAL_FLT:
    case APEX_VAL_DBL:
    case APEX_VAL_STR:
    case APEX_VAL_FN:
    case APEX_VAL_ARR:
    case APEX_VAL_TYPE:
    case APEX_VAL_OBJ:
        apexVM_pushbool(vm, true);
        break;

    case APEX_VAL_BOOL:
        apexVM_pushval(vm, value);
        break;

    case APEX_VAL_NULL:
        apexVM_pushbool(vm, false);
        break;
    }
    return 0;
}

/**
 * Returns the length of the provided value.
 *
 * If the value is an array, this is the number of elements in the array.
 * If the value is a string, this is the length of the string.
 *
 * If the value is of any other type, a runtime error is raised.
 *
 * @param vm A pointer to the virtual machine.
 * @param argc The number of arguments passed to the function.
 * @return Returns 0 on success, or 1 if an error occurs.
 */
static int std_len(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "std:len expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_ARR:
        apexVM_pushint(vm, value.arrval->entry_count);
        break;

    case APEX_VAL_STR:
        apexVM_pushint(vm, apexVal_str(value)->len);
        break;

    default:
        apexErr_runtime(vm, "cannot get length of %s", apexVal_typestr(value));
        return 1;
    }
    return 0;
}

apex_reglib(std, 
    apex_regfn("int", std_int),
    apex_regfn("str", std_str),
    apex_regfn("flt", std_flt),
    apex_regfn("dbl", std_dbl),
    apex_regfn("bool", std_bool),
    apex_regfn("len", std_len)
);