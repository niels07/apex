#include <stdio.h>
#include <stdlib.h>
#include "malloc.h"
#include "value.h"

const char *TYPE_STRING[] = {
    "int", "bool", "float", "string", "function"
};

void ApexValue_to_int(ApexValue *value, int intval) {
    value->data.int_val = intval;
    value->type = APEX_VALUE_TYPE_INT;
}

void ApexValue_to_string(char *str, ApexValue *value) {
    switch (value->type) {
    case APEX_VALUE_TYPE_INT:
        sprintf(str, "%d", value->data.int_val);
        break; 

    case APEX_VALUE_TYPE_FLT:
        sprintf(str, "%f", value->data.flt_val);
        break;

    case APEX_VALUE_TYPE_STR:
        sprintf(str, "%s", value->data.str_val);
        break;
    }
}

const char *ApexValue_get_type_name(ApexValue_type type) {
    return TYPE_STRING[type];
}

void ApexValue_make_int(ApexValue *value, int intval) {
    value->data.int_val = intval;
    value->type = APEX_VALUE_TYPE_INT;
}

void ApexValue_make_string(ApexValue *value, const char *str) {
    value->data.str_val = dupstr(str);
    value->type = APEX_VALUE_TYPE_STR;
}
