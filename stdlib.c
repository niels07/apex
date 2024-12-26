#include <stdio.h>
#include "stdlib.h"
#include "value.h"
#include "vm.h"
#include "string.h"
#include "util.h"
#include "error.h"

void apex_write(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    printf("%s", apexStr_valtostr(value));
}

void apex_print(ApexVM *vm) {
    apex_write(vm);
    printf("\n");
}

void apex_read(ApexVM *vm) {
    char *line = apexStr_readline(stdin);
    apexVM_pushval(vm, apexVal_makestr(line));
}

void apex_int(ApexVM *vm) {
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

    case APEX_VAL_STR: {
        int i;
        if (apexUtl_stoi(&i, apexVal_str(value))) {
            apexVM_pushint(vm, i);
        } else {
            apexErr_fatal(
                "cannot convert string \"%s\" to int on line %d", 
                value.strval,
                vm->lineno);
        }
        break;
    }

    case APEX_VAL_FN: 
        apexErr_fatal("cannot convert fn to int");
        break;

    case APEX_VAL_NULL:
        apexErr_fatal("cannot convert null to int");
        break;
    }
}

void apex_str(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    char *str = apexStr_valtostr(value);
    apexVM_pushstr(vm, str);
}

void apex_flt(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
        apexVM_pushflt(vm, (float)apexVal_int(value));
        break;

    case APEX_VAL_FLT:
        apexVM_pushval(vm, value);
        break;

    case APEX_VAL_STR: {
        float f;
        if (apexUtl_stof(&f, apexVal_str(value))) {
            apexVM_pushflt(vm, f);
        } else {
            apexErr_fatal(
                "cannot convert string \"%s\" to flt on line %d", 
                value.strval,
                vm->lineno);
        }
        break;
    }

   case APEX_VAL_BOOL:
        apexVM_pushflt(vm, apexVal_bool(value));
        break;

    case APEX_VAL_FN: 
        apexErr_fatal("cannot convert fn to flt");
        break;

    case APEX_VAL_NULL:
        apexErr_fatal("cannot convert null to flt");
        break;
    }
}

void apex_bool(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    switch (apexVal_type(value)) {
    case APEX_VAL_INT:
    case APEX_VAL_FLT:
    case APEX_VAL_STR:
    case APEX_VAL_FN:
        apexVM_pushbool(vm, TRUE);
        break;

    case APEX_VAL_BOOL:
        apexVM_pushval(vm, value);
        break;

    case APEX_VAL_NULL:
        apexVM_pushbool(vm, FALSE);
        break;
    }
}

StdLib apex_stdlib[] = {
    { "write", apex_write },
    { "print", apex_print },
    { "read", apex_read },
    { "int", apex_int },
    { "flt", apex_flt },
    { "str", apex_str },
    { "bool", apex_bool },
    { NULL, NULL }
};