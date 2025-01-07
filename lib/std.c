
#include <stdlib.h>
#include <stdio.h>

#include "vm.h"
#include "value.h"
#include "util.h"
#include "error.h"
#include "lib.h"


void std_int(ApexVM *vm) {
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
            exit(EXIT_FAILURE);
            apexErr_type(vm, "cannot convert string \"%s\" to int", apexVal_str(value)->value);
        }
        break;
    }

    default:
        apexErr_type(vm, "cannot convert %s to int", apexVal_typestr(value));
        break;
    }
}

void std_str(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    char *str = apexVal_tostr(value);
    apexVM_pushstr(vm, str);
}

void std_flt(ApexVM *vm) {
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
            apexErr_type(vm, "cannot convert string \"%s\" to flt", apexVal_str(value)->value);
        }
        break;
    }
   case APEX_VAL_BOOL:
        apexVM_pushflt(vm, apexVal_bool(value));
        break;

    default:
        apexErr_type(vm, "cannot convert %s to flt", apexVal_typestr(value));
        break;
    }
}

void std_dbl(ApexVM *vm) {
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
            apexErr_type(vm, "cannot convert string \"%s\" to dbl", apexVal_str(value)->value);
        }
        break;
    }
   case APEX_VAL_BOOL:
        apexVM_pushdbl(vm, apexVal_bool(value));
        break;
    default:
        apexErr_type(vm, "cannot convert %s to dbl", apexVal_typestr(value));
    }
}

void std_bool(ApexVM *vm) {
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
}

static void std_len(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_ARR:
        apexVM_pushint(vm, value.arrval->n);
        break;

    case APEX_VAL_STR:
        apexVM_pushint(vm, apexVal_str(value)->len);
        break;

    default:
        apexErr_type(vm, "cannot get length of %s", apexVal_typestr(value));
        break;
    }
}

apex_reglib(std, 
    apex_regfn("int", std_int),
    apex_regfn("str", std_str),
    apex_regfn("flt", std_flt),
    apex_regfn("dbl", std_dbl),
    apex_regfn("bool", std_bool),
    apex_regfn("len", std_len)
);