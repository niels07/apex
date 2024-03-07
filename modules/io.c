#include <stdio.h>

APEX_NEW_MODULE();

void ApexIO_print(ApexVm *vm, int argc) {
    int i;

    for (i = 0; i < argc; i++) {
        ApexValue *value = ApexVm_pop(vm);

        switch (APEX_TYPE_OF(value)) {
        case APEX_VALUE_TYPE_INT:
            fprintf(stdout, "%d\n", APEX_VALUE_INT(value));
            break;

        case APEX_VALUE_TYPE_FLT:
            fprintf(stdout, "%f\n", APEX_VALUE_FLT(value));
            break;

        case APEX_VALUE_TYPE_STR:
            fprintf(stdout, "%d\n", APEX_VALUE_STR(value));
        default: 
            break;
        }
    }
}

APEX_EXPORT {
    APEX_EXPORT_FUNC("print", ApexIO_print),
    APEX_EXPORT_END
};