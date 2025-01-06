#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include "string.h"

typedef enum {
    APEX_VAL_INT,
    APEX_VAL_FLT,
    APEX_VAL_DBL,
    APEX_VAL_STR,
    APEX_VAL_BOOL,
    APEX_VAL_FN,
    APEX_VAL_ARR,
    APEX_VAL_TYPE,
    APEX_VAL_OBJ,
    APEX_VAL_NULL
} ValueType;

typedef struct {
    const char *name;
    const char **params;
    int param_n;
    int addr;
    int refcount;
} Fn;

typedef struct Array Array;
typedef struct ApexObject ApexObject;

typedef struct {
    ValueType type;
    union {
        int intval;
        float fltval;
        double dblval;
        ApexString *strval;
        bool boolval;
        Fn *fnval;
        Array *arrval;
        ApexObject *objval;
    };
} ApexValue;

typedef struct ArrayEntry {
    ApexValue key;
    ApexValue value;
    struct ArrayEntry *next;
} ArrayEntry;

struct Array {
    ArrayEntry **entries;
    int size;
    int n;
    int refcount;
};

typedef struct ApexObjectEntry {
    const char *key;
    ApexValue value;
    struct ApexObjectEntry *next;
} ApexObjectEntry;

struct ApexObject {
    ApexObjectEntry **entries;
    int size;
    int n;
    int refcount;
    const char *name;
};

#define apexVal_int(v) (v.intval)
#define apexVal_flt(v) (v.fltval)
#define apexVal_dbl(v) (v.dblval)
#define apexVal_str(v) (v.strval)
#define apexVal_bool(v) (v.boolval)
#define apexVal_fn(v) (v.fnval)
#define apexVal_type(v) (v.type)

extern Fn *apexVal_newfn(const char *name, const char **params, int param_n, int addr);
extern const char *apexVal_typestr(ApexValue value);
extern char *apexVal_tostr(ApexValue value);
extern ApexValue apexVal_makeint(int value);
extern ApexValue apexVal_makeflt(float value);
extern ApexValue apexVal_makedbl(double value);
extern ApexValue apexVal_makestr(ApexString *value);
extern ApexValue apexVal_makebool(bool value);
extern ApexValue apexVal_makefn(Fn *function);
extern ApexValue apexVal_makearr(Array *arr);
extern ApexValue apexVal_maketype(ApexObject *obj);
extern ApexValue apexVal_makeobj(ApexObject *obj);
extern ApexValue apexVal_makenull(void);
extern int apexVal_arrlen(ApexValue value);
extern void apexVal_retain(ApexValue value);
extern void apexVal_release(ApexValue value);
extern Array *apexVal_newarray(void);
extern ApexObject *apexVal_newobject(void);
extern void apexVal_freearray(Array *array);
extern void apexVal_freeobject(ApexObject *object);
extern void apexVal_arrayset(Array *array, ApexValue key, ApexValue value);
extern void apexVal_objectset(ApexObject *object, const char *key, ApexValue value);
extern bool apexVal_arrayget(ApexValue *value, Array *array, const ApexValue key);
extern bool apexVal_objectget(ApexValue *value, ApexObject *object, const char *key);
extern void apexVal_arraydel(Array *array, const ApexValue key);

#endif
