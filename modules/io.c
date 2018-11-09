
#include "api.h"

void apex_io_print(void) {
    ApexValue *value = apex_pop();
}

APEX_MODULE = "io";

APEX_EXPORT = {
    { "print", apex_io_print }
};

