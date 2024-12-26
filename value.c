#include <string.h>
#include <stdlib.h>
#include "value.h"

const char *get_value_type_str(ValueType type) {
    switch (type) {
    case APEX_VAL_INT:
        return "int";
    case APEX_VAL_FLT:
        return "float";
    case APEX_VAL_STR:
        return "string";
    case APEX_VAL_BOOL:
        return "bool";
    default:
        break;
    }
    return "null";
}

ApexValue apexVal_makeint(int value) {
    ApexValue v;
    v.type = APEX_VAL_INT;
    v.intval = value;
    return v;
}

ApexValue apexVal_makeflt(float value) {
    ApexValue v;
    v.type = APEX_VAL_FLT;
    v.fltval = value;
    return v;
}

/**
 * Creates an ApexValue with the given string value.
 *
 * @param value The string value to associate with the ApexValue.
 *
 * @return An ApexValue with the given string value.
 */
ApexValue apexVal_makestr(char *value) {
    ApexValue v;
    v.type = APEX_VAL_STR;
    v.strval = value;
    return v;
}

ApexValue apexVal_makebool(Bool value) {
    ApexValue v;
    v.type = APEX_VAL_BOOL;
    v.boolval = value;
    return v;
}

ApexValue apexVal_makefn(Fn *fn) {
    ApexValue v;
    v.type = APEX_VAL_FN;
    v.fnval = fn;
    return v;
}

ApexValue apexVal_makenull(void) {
    ApexValue v;
    v.type = APEX_VAL_NULL;
    return v;
}

void free_value(ApexValue value) {
    if (value.type == APEX_VAL_FN) {
        free(value.fnval->params);
        free(value.fnval);        
    }
}