#include <stdio.h>
#include "value.h"

static const char *TYPE_STR[] = {
    "int", "flt", "str", "obj", "func", "none"
};

void apex_value(ApexValue *value, int intval) {
    value->data.intval = intval;
    value->type = APEX_TYPE_INT;
}

const char *apex_type_str(ApexType type) {
    return TYPE_STR[type];
}

void apex_value_to_string(ApexValue *value, char *str) {
    switch (value->type) {
    case APEX_TYPE_INT:
        sprintf(str, "%d", APEX_VALUE_INT(value));
        break; 

    case APEX_TYPE_FLT:
        sprintf(str, "%f", APEX_VALUE_FLT(value));
        break;

    case APEX_TYPE_STR:
        sprintf(str, "%s", APEX_VALUE_STR(value));
        break;
    }
}

void apex_value_make_int(ApexValue *value, int intval) {
    APEX_VALUE_INT(value) = intval;
    value->type = APEX_TYPE_INT;
}

void apex_value_make_func(ApexValue *value, ApexFunc *func) {
    APEX_VALUE_FUNC(value) = func;
    value->type = APEX_TYPE_INT;
}


