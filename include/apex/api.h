#ifndef APEX_API_H
#define APEX_API_H

struct ApexApiExports {
    ApexType type;
    char *name;
    int intval;
    float fltval;
    char *strval;
    int obj;
    ApexFunc *func;
};

struct ApexApiMethods {
    ApexValue *(*pop)(void);
    ApexValue *(*top)(void);
};

#define APEX_API_METHODS apex_api_methods
#define APEX_API_EXPORTS apex_api_exports

#define APEX_NEW_MODULE() struct ApexApiMethods APEX_API_METHODS = {NULL, NULL}
#define APEX_POP() APEX_API_METHODS.pop()
#define APEX_TOP() APEX_API_METHODS.top()
#define APEX_EXPORT struct ApexApiExports APEX_API_EXPORTS[] =

#define APEX_EXPORT_END { 0, NULL, 0, 0.0, NULL, 0, NULL }

#define APEX_EXPORT_FUNC(name, func) { APEX_TYPE_FUNC, name, 0, 0.0, NULL, 0, func }

#endif 