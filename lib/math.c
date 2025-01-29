#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "apexVM.h"
#include "apexVal.h"
#include "apexErr.h"
#include "apexLib.h"

ApexValue math_pi = {
    .type = APEX_VAL_DBL,
    .dblval = 3.141592653589793238462643383279502884
};

ApexValue math_huge = {
    .type = APEX_VAL_DBL,
    .dblval = HUGE_VAL
};

/**
 * Returns a random integer or double value
 *
 * If given no arguments, returns a random double between 0 and 1.
 * If given one argument, returns a random integer between 1 and the argument.
 * If given two arguments, returns a random integer between the two bounds.
 *
 * @param vm The virtual machine to use
 * @param argc The number of arguments to the function
 * @return 0 on success, 1 on error
 */
int math_random(ApexVM *vm, int argc) {
    if (argc > 2) {
        apexErr_runtime(vm, "math:random expects at most 2 arguments");
        return 1;
    }

    // Seed the random number generator once
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    if (argc == 0) {
        // Case 1: No arguments, return a random double between 0 and 1
        double random_value = (double)rand() / (double)RAND_MAX;
        apexVM_pushdbl(vm, random_value);
    } else if (argc == 1) {
        // Case 2: One argument, return a random integer between 1 and the argument
        ApexValue arg = apexVM_pop(vm);
        if (apexVal_type(arg) != APEX_VAL_INT) {
            apexErr_runtime(vm, "math:random expects an integer as the first argument");
            return 1;
        }
        int upper_bound = arg.intval;
        if (upper_bound < 1) {
            apexErr_runtime(vm, "math:random upper bound must be at least 1");
            return 1;
        }
        int random_value = (rand() % upper_bound) + 1;
        apexVM_pushint(vm, random_value);
    } else if (argc == 2) {
        // Case 3: Two arguments, return a random integer between the two bounds
        ApexValue upper_val = apexVM_pop(vm);
        ApexValue lower_val = apexVM_pop(vm);

        if (apexVal_type(lower_val) != APEX_VAL_INT || apexVal_type(upper_val) != APEX_VAL_INT) {
            apexErr_runtime(vm, "math:random expects two integers as arguments");
            return 1;
        }

        int lower_bound = lower_val.intval;
        int upper_bound = upper_val.intval;

        if (lower_bound > upper_bound) {
            apexErr_runtime(vm, "math:random lower bound must be less than or equal to upper bound");
            return 1;
        }

        int random_value = (rand() % (upper_bound - lower_bound + 1)) + lower_bound;
        apexVM_pushint(vm, random_value);
    }

    return 0;
}

/**
 * Computes the absolute value of an integer.
 *
 * This function takes a single integer argument and returns its absolute value.
 * If the argument is not an integer or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_abs(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:abs expects exactly 1 argument");
        return 1;
    }

    ApexValue value = apexVM_pop(vm);
    if (apexVal_type(value) != APEX_VAL_INT) {
        apexErr_runtime(vm, "math:abs expects argument to be int");
        return 1;
    }

    int x = apexVal_int(value);
    int result = abs(x);
    apexVM_pushint(vm, result);
    return 0;
}

/**
 * Computes the absolute value of a number.
 *
 * This function takes a single argument and returns its absolute value.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_fabs(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:fabs expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:fabs expects argument to be int, flt or dbl");
        return 1;
    }
    double result = fabs(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the cosine of an angle.
 *
 * This function takes a single argument in radians and returns its cosine.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_cos(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:cos expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:cos expects argument to be int, flt or dbl");
        return 1;
    }
    double result = cos(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the hyperbolic cosine of an angle.
 *
 * This function takes a single argument in radians and returns its hyperbolic cosine.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_cosh(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:cosh expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:cosh expects argument to be int, flt or dbl");
        return 1;
    }
    double result = cosh(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the arc cosine of an angle.
 *
 * This function takes a single argument in radians and returns its arc cosine.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_acos(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:acos expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:acos expects argument to be int, flt or dbl");
        return 1;
    }
    double result = acos(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the sine of an angle.
 *
 * This function takes a single argument in radians and returns its sine.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_sin(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:sin, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:sin expects argument to be int, flt or dbl");
        return 1;
    }
    double result = sin(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the arc sine of an angle.
 *
 * This function takes a single argument in radians and returns its arc sine.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_asin(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:asin, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:asin expects argument to be int, flt or dbl");
        return 1;
    }
    double result = asin(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the tangent of an angle.
 *
 * This function takes a single argument in radians and returns its tangent.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_tan(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:tan, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:tan expects argument to be int, flt or dbl");
        return 1;
    }
    double result = tan(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the arc tangent of a number.
 *
 * This function takes a single argument and returns its arc tangent.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_atan(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:atan, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:atan expects argument to be int, flt or dbl");
        return 1;
    }
    double result = atan(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the arc tangent of a number given two arguments.
 *
 * This function takes two arguments and returns the arc tangent of the division of the first by the second.
 * The arguments can be integers, floats, or doubles.
 * If the arguments are not of one of the above types, or if the number of arguments is not exactly two,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 2.
 * @return 0 on success, 1 on error.
 */
int math_atan2(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "math:atan2, expects exactly 2 arguments");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:atan2 expects second argument to be int, flt or dbl");
        return 1;
    }
    double y;
    value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        y = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        y = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        y = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:atan2 expects first argument to be int, flt or dbl");
        return 1;
    }
    double result = atan2(x, y);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Rounds a number up to the nearest integer.
 *
 * This function takes a single argument and rounds it up to the nearest integer.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_ceil(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:ceil, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:ceil expects argument to be int, flt or dbl");
        return 1;
    }
    double result = ceil(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Rounds a number down to the nearest integer.
 *
 * This function takes a single argument and rounds it down to the nearest integer.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_floor(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:floor, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:floor expects argument to be int, flt or dbl");
        return 1;
    }
    double result = floor(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the exponential of a number.
 *
 * This function takes a single argument and returns its exponential.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_exp(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:exp, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:exp expects argument to be int, flt or dbl");
        return 1;
    }
    double result = exp(x);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Computes the floating-point remainder of the division of two numbers.
 *
 * This function takes two arguments and returns the remainder of the division
 * of the first argument by the second. The arguments can be integers, floats, or doubles.
 * If one of the arguments is not of one of the above types, or if the number of arguments
 * is not exactly two, a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 2.
 * @return 0 on success, 1 on error.
 */
int math_fmod(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "math:fmod, expects exactly 2 arguments");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:fmod expects second argument to be int, flt or dbl");
        return 1;
    }
    double y;
    value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        y = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        y = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        y = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:fmod expects first argument to be int, flt or dbl");
        return 1;
    }
    double result = fmod(x, y);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Decomposes a floating-point number into its fractional and integer parts.
 *
 * This function takes a single argument and returns a two-element array containing
 * the integer and fractional parts of the argument.
 * The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments is not exactly one,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_frexp(ApexVM *vm, int argc) {
     if (argc != 1) {
        apexErr_runtime(vm, "math:frexp, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:frexp expects argument to be int, flt or dbl");
        return 1;
    }
    int intpart;
    double fractpart = frexp(x, &intpart);
    ApexArray *array = apexVal_newarray();
    apexVal_arrayset(array, apexVal_makeint(0), apexVal_makeint(intpart));
    apexVal_arrayset(array, apexVal_makeint(1), apexVal_makedbl(fractpart));
    apexVM_pusharr(vm, array);
    return 0;
}

/**
 * Multiply a floating-point number by a power of 2.
 *
 * This function takes two arguments, a floating-point number and an integer exponent.
 * The function returns the result of multiplying the floating-point number by 2 raised to the power of the exponent.
 * The first argument can be an integer, float, or double.
 * The second argument must be an integer.
 * If the arguments are not of the above types, or if the number of arguments is not exactly two,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 2.
 * @return 0 on success, 1 on error.
 */
int math_ldexp(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "math:ldexp, expects exactly 2 arguments");
        return 1;
    }    
    ApexValue value = apexVM_pop(vm);
    if (apexVal_type(value) != APEX_VAL_INT) {   
        apexErr_runtime(vm, "math:ldexp expects second argument to be int, flt or dbl");
        return 1;
    }
    int exponent = apexVal_int(value);
    double x;
    value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:ldexp expects first argument to be int, flt or dbl");
        return 1;
    }

    double result = ldexp(x, exponent);
    apexVM_pushdbl(vm, result);
    return 0;
}

/**
 * Decomposes a floating-point number into its integer and fractional parts.
 *
 * This function takes a single argument and returns a two-element array containing
 * the integer and fractional parts of the argument. The integer part is truncated
 * towards zero. The argument can be an integer, float, or double.
 * If the argument is not of one of the above types, or if the number of arguments
 * is not exactly one, a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be 1.
 * @return 0 on success, 1 on error.
 */
int math_modf(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "math:modf, expects exactly 1 argument");
        return 1;
    }
    double x;
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        x = (double)apexVal_int(value);
        break;
    case APEX_VAL_FLT:
        x = (double)apexVal_flt(value);
        break;
    case APEX_VAL_DBL:
        x = apexVal_dbl(value);
        break;
    default:
        apexErr_runtime(vm, "math:modf expects argument to be int, flt or dbl");
        return 1;
    }
    double intpart;
    double fractpart = modf(x, &intpart);
    ApexArray *array = apexVal_newarray();
    apexVal_arrayset(array, apexVal_makeint(0), apexVal_makeint((int)intpart));
    apexVal_arrayset(array, apexVal_makeint(1), apexVal_makedbl(fractpart));
    apexVM_pusharr(vm, array);
    return 0;
}

/**
 * Returns the maximum value from a list of numbers.
 *
 * This function takes one or more arguments and returns the maximum value among them.
 * The arguments can be integers, floats, or doubles.
 * If any argument is not of one of the above types, or if no arguments are provided,
 * a runtime error is raised.
 *
 * @param vm The virtual machine to use.
 * @param argc The number of arguments to the function, expected to be at least 1.
 * @return 0 on success, 1 on error.
 */
int math_max(ApexVM *vm, int argc) {
    if (argc < 1) {
        apexErr_runtime(vm, "math:max, expects at least 1 argument");
        return 1;
    }
    int numbers[argc];
    double max;
    bool max_set = false;
    ApexValueType type;
    
    while (argc--) {
        ApexValue value = apexVM_pop(vm);
        double num;
        switch (apexVal_type(value)) {
        case APEX_VAL_INT:
            num = (double)apexVal_int(value);
            break;
        case APEX_VAL_FLT:
            num = (double)apexVal_flt(value);
            break;
        case APEX_VAL_DBL:
            num = apexVal_dbl(value);
            break;
        default:
            apexErr_runtime(vm, "math:max expects arguments to be int, flt or dbl");
            return 1;
        }
        if (!max_set || num > max) {
            max = num;
            max_set = true;
            type = apexVal_type(value);
        }
    }

    switch (type) {
    case APEX_VAL_INT:
        apexVM_pushint(vm, (int)max);
        break;
    case APEX_VAL_FLT:
        apexVM_pushflt(vm, (float)max);
        break;
    case APEX_VAL_DBL:
        apexVM_pushdbl(vm, max);
        break;
    default:
        break;
    }
    return 0;
}

int math_min(ApexVM *vm, int argc) {
    if (argc < 1) {
        apexErr_runtime(vm, "math:max, expects at least 1 argument");
        return 1;
    }
    int numbers[argc];
    double min;
    bool min_set = false;
    ApexValueType type;
    
    while (argc--) {
        ApexValue value = apexVM_pop(vm);
        double num;
        switch (apexVal_type(value)) {
        case APEX_VAL_INT:
            num = (double)apexVal_int(value);
            break;
        case APEX_VAL_FLT:
            num = (double)apexVal_flt(value);
            break;
        case APEX_VAL_DBL:
            num = apexVal_dbl(value);
            break;
        default:
            apexErr_runtime(vm, "math:max expects arguments to be int, flt or dbl");
            return 1;
        }
        if (!min_set || num < min) {
            min = num;
            min_set = true;
            type = apexVal_type(value);
        }
    }

    switch (type) {
    case APEX_VAL_INT:
        apexVM_pushint(vm, (int)min);
        break;
    case APEX_VAL_FLT:
        apexVM_pushflt(vm, (float)min);
        break;
    case APEX_VAL_DBL:
        apexVM_pushdbl(vm, min);
        break;
    default:
        break;
    }
    return 0;
}

apex_reglib(math,
    apex_regvar("pi", math_pi),
    apex_regvar("huge", math_huge),
    apex_regfn("random", math_random),
    apex_regfn("abs", math_abs),
    apex_regfn("fabs", math_fabs),
    apex_regfn("cos", math_cos),
    apex_regfn("cosh", math_cosh),
    apex_regfn("acos", math_acos),
    apex_regfn("sin", math_sin),
    apex_regfn("asin", math_asin),
    apex_regfn("tan", math_tan),
    apex_regfn("atan", math_atan),
    apex_regfn("atan2", math_atan2),
    apex_regfn("ceil", math_ceil),
    apex_regfn("floor", math_floor),
    apex_regfn("exp", math_exp),
    apex_regfn("fmod", math_fmod),
    apex_regfn("frexp", math_frexp),
    apex_regfn("ldexp", math_ldexp),
    apex_regfn("modf", math_modf),
    apex_regfn("max", math_max)
)