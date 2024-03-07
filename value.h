#ifndef APEX_VALUE_H
#define APEX_VALUE_H

#include "types.h"

#ifndef APEX_VM_DEFINED
#define APEX_VM_DEFINED
    typedef struct ApexVm ApexVm;
#endif

#ifndef APEX_VALUE_DEFINED
#define APEX_VALUE_DEFINED
    typedef struct ApexValue ApexValue;
#endif

#define APEX_VALUE_INT(value) value->data.int_val
#define APEX_VALUE_BOOL(value) value->data.bool_val
#define APEX_VALUE_FLT(value) value->data.flt_val
#define APEX_VALUE_STR(value) value->data.str_val

typedef enum {
    APEX_VALUE_TYPE_INT,
    APEX_VALUE_TYPE_BOOL,
    APEX_VALUE_TYPE_FLT,
    APEX_VALUE_TYPE_STR,
    APEX_VALUE_TYPE_FUNC
} ApexValue_type;

typedef void (*ApexValue_function)(ApexVm *, int);

struct ApexValue {
    union {
        int int_val;
        bool bool_val;
        char *str_val;
        float flt_val;
        ApexValue_function func_val;
        void *value;
    } data;
    ApexValue_type type;
};

extern const char *ApexValue_get_type_name(ApexValue_type);
extern void ApexValue_make_string(ApexValue *, const char *);
extern void ApexValue_make_int(ApexValue *, int);

#endif