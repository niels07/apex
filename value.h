#ifndef VALUE_H
#define VALUE_H

#include "bool.h"

typedef enum {
    APEX_VAL_INT,
    APEX_VAL_FLT,
    APEX_VAL_STR,
    APEX_VAL_BOOL,
    APEX_VAL_FN,
    APEX_VAL_NULL
} ValueType;

typedef struct {
    const char *name;
    const char **params;
    int param_n;
    int addr;
} Fn;

typedef struct {
    ValueType type;
    union {
        int intval;
        float fltval;
        char *strval;
        Bool boolval;
        Fn *fnval;
    };
} ApexValue;

#define apexVal_int(v) (v.intval)
#define apexVal_flt(v) (v.fltval)
#define apexVal_str(v) (v.strval)
#define apexVal_bool(v) (v.boolval)
#define apexVal_fn(v) (v.fnval)
#define apexVal_type(v) (v.type)

extern const char *get_value_type_str(ValueType type);
extern ApexValue apexVal_makeint(int value);
extern ApexValue apexVal_makeflt(float value);
extern ApexValue apexVal_makestr(char *value);
extern ApexValue apexVal_makebool(Bool value);
extern ApexValue apexVal_makefn(Fn *function);
extern ApexValue apexVal_makenull(void);
extern void free_value(ApexValue value);

#endif
