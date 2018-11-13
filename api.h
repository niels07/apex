#ifndef APEX_API_H
#define APEX_API_H

#include "value.h"

#define APEX_EXPORT_BEGIN struct __ApexExport __apex_export[] = {
#define APEX_EXPORT_END { 0, NULL, 0, 0.0, NULL, 0, NULL } }

#define APEX_EXPORT_FUNC(name, func) \
    { APEX_TYPE_FUNC, name, 0, 0.0, NULL, 0, func },

struct __ApexExport {
    ApexType type;
    char *name;
    int intval;
    float fltval;
    char *strval;
    int obj;
    ApexFunc *func;
};

extern ApexValue *apex_pop(void);
extern ApexValue *apex_top(void);

#define APEX_MODULE const char *__apex_module

#endif
