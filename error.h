#ifndef APX_ERROR_H
#define APX_ERROR_H

#define APEX_ERROR_MAX 1024

typedef enum {
    APEX_ERROR_CODE_INTERNAL,
    APEX_ERROR_CODE_IO,
    APEX_ERROR_CODE_NOMEM,
    APEX_ERROR_CODE_SYNTAX,
    APEX_ERROR_CODE_TYPE,
    APEX_ERROR_CODE_RUNTIME,
    APEX_ERROR_CODE_ARGUMENT,
    APEX_ERROR_CODE_REFERENCE
} ApexErrorCode;

typedef struct ApexErrorHandler ApexErrorHandler;
typedef void (ApexErrorCallback)(ApexErrorHandler *);

extern void apex_error_throw(ApexErrorCode, const char *, ...);
extern void apex_error_set_handler(ApexErrorCode, ApexErrorCallback *, void *);
extern void apex_error_internal(const char *, ...);
extern void apex_error_runtime(const char *, ...);
extern void apex_error_syntax(const char *, ...);
extern void apex_error_type(const char *, ...);
extern void apex_error_nomem(const char *, ...);
extern void apex_error_io(const char *, ...);
extern void apex_error_argument(const char *, ...);
extern void apex_error_reference(const char *, ...);
extern void apex_error_print(ApexErrorHandler *);

#endif 
