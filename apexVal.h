#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include "apexStr.h"

struct ApexVM; 
typedef struct ApexVM ApexVM;

/**
 * Enum type to represent the type of an ApexValue.
 */
typedef enum {
    APEX_VAL_INT, /** Integer value */
    APEX_VAL_FLT, /** Float value */
    APEX_VAL_DBL, /** Double value */
    APEX_VAL_STR, /** String value */
    APEX_VAL_BOOL, /** Boolean value */
    APEX_VAL_FN, /** Function value */
    APEX_VAL_CFN, /** CFunction value */
    APEX_VAL_PTR, /** Pointer value */
    APEX_VAL_ARR, /** Array value */
    APEX_VAL_TYPE, /** Type value */
    APEX_VAL_OBJ, /** Object value */
    APEX_VAL_NULL /** Null value */
} ApexValueType;

/**
 * CFunction struct to represent a C function
 */
typedef struct {
    char *name; /** The cfunction name */
    int (*fn)(ApexVM *, int); /** The cfunction pointer */
} ApexCfn;

/**
 * Function struct to represent an Apex function
 */
typedef struct {
    const char *name; /** The function name */
    char **params; /** The function parameters */
    int argc; /** The number of parameters */
    int addr; /** The address of the function */
    int refcount; /** The number of references to the function */
    bool have_variadic; /** Whether the function has variadic arguments */
} ApexFn;

/**
 * Array struct to represent an array value
 */
typedef struct ApexArray ApexArray;
/**
 * Object struct to represent an object value
 */
typedef struct ApexObject ApexObject;

/**
 * Union to represent a value of any type
 */
typedef struct {
    ApexValueType type; /** The type of the value */
    union {
        int intval; /** Integer value */
        float fltval; /** Float value */
        double dblval; /** Double value */
        ApexString *strval; /** String value */
        bool boolval; /** Boolean value */
        ApexFn *fnval; /** Function value */
        ApexCfn cfnval; /** CFunction value */
        void *ptrval; /** Pointer value */
        ApexArray *arrval; /** Array value */
        ApexObject *objval; /** Object value */
    };
} ApexValue;

/**
 * ArrayEntry struct to represent an entry in an array
 */
typedef struct ApexArrayEntry {
    ApexValue key; /** The key of the entry */
    ApexValue value; /** The value of the entry */
    struct ApexArrayEntry *next; /** The next entry */
    int index; /** The index of the entry */
} ApexArrayEntry;

/**
 * ApexArray struct to represent an array
 */
struct ApexArray {
    ApexArrayEntry **entries; /** The entries of the array */
    ApexArrayEntry **iter; /** The iterator of the array */
    int entry_size; /** The number of entries */
    int entry_count; /** The number of entries */
    int iter_size; /** The number of entries in the iterator */
    int iter_count; /** The number of entries in the iterator */
    int refcount; /** The number of references to the array */
    bool is_assigned; /** Whether the array has been assigned */
};

/**
 * ObjectEntry struct to represent an entry in an object
 */
typedef struct ApexObjectEntry {
    const char *key; /** The key of the entry */
    ApexValue value; /** The value of the entry */
    struct ApexObjectEntry *next; /** The next entry */
} ApexObjectEntry;

/**
 * ApexObject struct to represent an object
 */
struct ApexObject {
    ApexObjectEntry **entries; /** The entries of the object */
    int size; /** The size of the object */
    int count; /** The number of entries */
    int refcount; /** The number of references to the object */
    const char *name; /** The name of the object */
};

#define apexArray_each(arr) for (int _iter = 0; _iter < arr->iter_count;)
#define apexArray_next(arr) (arr->iter[_iter++])

/**
 * Get the integer value from an ApexValue.
 *
 * @param v ApexValue containing an integer value.
 * @return The integer value contained in the ApexValue.
 */
#define apexVal_int(v) (v.intval)

/**
 * Get the float value from an ApexValue.
 *
 * @param v ApexValue containing a float value.
 * @return The float value contained in the ApexValue.
 */
#define apexVal_flt(v) (v.fltval)

/**
 * Get the double value from an ApexValue.
 *
 * @param v ApexValue containing a double value.
 * @return The double value contained in the ApexValue.
 */
#define apexVal_dbl(v) (v.dblval)

/**
 * Get the ApexString from an ApexValue.
 *
 * @param v ApexValue containing an ApexString.
 * @return The ApexString contained in the ApexValue.
 */
#define apexVal_str(v) (v.strval)

/**
 * Get the boolean value from an ApexValue.
 *
 * @param v ApexValue containing a boolean value.
 * @return The boolean value contained in the ApexValue.
 */
#define apexVal_bool(v) (v.boolval)

/**
 * Get the ApexArray from an ApexValue.
 *
 * @param v ApexValue containing an ApexArray.
 * @return The ApexArray contained in the ApexValue.
 */
#define apexVal_array(v) (v.arrval)

/**
 * Get the ApexFn from an ApexValue.
 *
 * @param v ApexValue containing an ApexFn.
 * @return The ApexFn contained in the ApexValue.
 */
#define apexVal_fn(v) (v.fnval)

/**
 * Get the ApexValueType from an ApexValue.
 *
 * @param v ApexValue containing an ApexValueType.
 * @return The ApexValueType contained in the ApexValue.
 */
#define apexVal_type(v) (v.type)

extern ApexFn *apexVal_newfn(const char *name, char **params, int argc, bool have_variadic, int addr);
extern ApexCfn apexVal_newcfn(char *name, int (*fn)(ApexVM *, int));
extern const char *apexVal_typestr(ApexValue value);
extern ApexString *apexVal_tostr(ApexValue value);
extern void apexVal_setassigned(ApexValue value, bool is_assigned);
extern bool apexVal_isassigned(ApexValue value);
extern ApexValue apexVal_makeint(int value);
extern ApexValue apexVal_makeflt(float value);
extern ApexValue apexVal_makedbl(double value);
extern ApexValue apexVal_makestr(ApexString *value);
extern ApexValue apexVal_makebool(bool value);
extern ApexValue apexVal_makefn(ApexFn *function);
extern ApexValue apexVal_makecfn(ApexCfn cfn);
extern ApexValue apexVal_makearr(ApexArray *arr);
extern ApexValue apexVal_maketype(ApexObject *obj);
extern ApexValue apexVal_makeobj(ApexObject *obj);
extern ApexValue apexVal_makeptr(void *ptr);
extern ApexValue apexVal_makenull(void);
extern bool apexVal_tobool(ApexValue value);
extern int apexVal_arrlen(ApexValue value);
extern void apexVal_retain(ApexValue value);
extern void apexVal_release(ApexValue value);
extern ApexArray *apexVal_newarray(void);
extern ApexObject *apexVal_newobject(const char *name);
extern ApexObject *apexVal_objectcpy(ApexObject *object);
extern void apexVal_freearray(ApexArray *array);
extern void apexVal_freeobject(ApexObject *object);
extern void apexVal_arrayset(ApexArray *array, ApexValue key, ApexValue value);
extern void apexVal_objectset(ApexObject *object, const char *key, ApexValue value);
extern bool apexVal_arrayget(ApexValue *value, ApexArray *array, const ApexValue key);
extern bool apexVal_objectget(ApexValue *value, ApexObject *object, const char *key);
extern void apexVal_arraydel(ApexArray *array, const ApexValue key);

#endif
