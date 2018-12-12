#include <stdio.h>
#include <apex.h>

APEX_NEW_MODULE();

void apex_io_print(int argc) {
    int i;

    for (i = 0; i < argc; i++) {
        ApexValue *value = APEX_POP();

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

APEX_EXPORT {
    APEX_EXPORT_FUNC("print", apex_io_print),
    APEX_EXPORT_END
};