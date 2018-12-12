#ifndef APX_VALUE_H
#define APX_VALUE_H

typedef enum {
    APEX_TYPE_INT,
    APEX_TYPE_FLT,
    APEX_TYPE_STR,
    APEX_TYPE_OBJ,
    APEX_TYPE_FUNC,
    APEX_TYPE_NONE
} ApexType;

typedef void ApexFunc(int);

typedef struct ApexValue {
    ApexType type;
    union {
        int intval;
        float fltval;
        char *strval;
        int obj;
        ApexFunc *func;
    } data;
} ApexValue;

extern void apex_value_make_int(ApexValue *, int);
extern void apex_value_make_func(ApexValue *, ApexFunc);
extern void apex_value_to_string(char *, ApexValue *);
extern const char *apex_type_get_name(ApexType);

#define APEX_VALUE_INT(value) ((value)->data.intval)
#define APEX_VALUE_FLT(value) ((value)->data.fltval)
#define APEX_VALUE_STR(value) ((value)->data.strval)
#define APEX_VALUE_FUNC(value) ((value)->data.func)
#define APEX_TYPE_OF(value) ((value)->type)

#endif

