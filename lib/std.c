
#include <stdlib.h>
#include <stdio.h>

#include "vm.h"
#include "apexVal.h"
#include "util.h"
#include "error.h"
#include "lib.h"


int std_int(ApexVM *vm) {
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
        if (apexUtl_stoi(&i, apexVal_str(value)->value)) {
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

int std_str(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    char *str = apexVal_tostr(value);
    apexVM_pushstr(vm, str);
    return 0;
}

int std_flt(ApexVM *vm) {
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
        if (apexUtl_stof(&f, apexVal_str(value)->value)) {
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

int std_dbl(ApexVM *vm) {
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
        if (apexUtl_stod(&d, apexVal_str(value)->value)) {
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

int std_bool(ApexVM *vm) {
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

static int std_len(ApexVM *vm) {
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
    apex_regfn("int", std_int, 1),
    apex_regfn("str", std_str, 1),
    apex_regfn("flt", std_flt, 1),
    apex_regfn("dbl", std_dbl, 1),
    apex_regfn("bool", std_bool, 1),
    apex_regfn("len", std_len, 1)
);