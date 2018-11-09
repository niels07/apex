#ifndef APEX_API_H
#define APEX_API_H

#include "value.h"

#define APEX_EXPORT struct __ApexExport __apex_export[]
#define APEX_MODULE const char *__apex_module

struct __ApexExport {
    char *name;
    ApexFunc *func;
};

extern ApexValue *apex_pop(void);
extern ApexValue *apex_top(void);

#endif
