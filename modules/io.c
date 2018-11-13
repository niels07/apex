#include <stdio.h>
#include "api.h"

void apex_io_print(int argc) {
    int i;

    for (i = 0; i < argc; i++) {
        ApexValue *value = apex_pop();

        switch (APEX_TYPE_OF(value)) {
        case APEX_TYPE_INT:
            printf("%d\n", APEX_VALUE_INT(value));
            break;

        case APEX_TYPE_FLT:
            printf("%f\n", APEX_VALUE_FLT(value));
            break;

        default: 
            break;
        }
    }
}

APEX_MODULE = "io";

APEX_EXPORT_BEGIN

    APEX_EXPORT_FUNC ("print", apex_io_print )

APEX_EXPORT_END;

