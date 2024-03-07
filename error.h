#ifndef APX_ERROR_H
#define APX_ERROR_H

#define ApexError_MAX 1024

typedef enum {
    APEX_ERROR_CODE_INTERNAL,
    APEX_ERROR_CODE_IO,
    APEX_ERROR_CODE_MEMORY,
    APEX_ERROR_CODE_SYNTAX,
    APEX_ERROR_CODE_TYPE,
    APEX_ERROR_CODE_RUNTIME,
    APEX_ERROR_CODE_ARGUMENT,
    APEX_ERROR_CODE_REFERENCE,
    APEX_ERROR_CODE_COMPILER,
    APEX_ERROR_CODE_OUT_OF_BOUNDS
} ApexErrorCode;

typedef struct ApexErrorHandler ApexErrorHandler;
typedef void (ApexErrorCallback)(ApexErrorHandler *);

extern void ApexError_throw(ApexErrorCode, const char *, ...);
extern void ApexError_set_handler(ApexErrorCode, ApexErrorCallback *, void *);
extern void ApexError_internal(const char *, ...);
extern void ApexError_runtime(const char *, ...);
extern void ApexError_syntax(const char *, ...);
extern void ApexError_type(const char *, ...);
extern void ApexError_out_of_bounds(const char *, ...);
extern void ApexError_io(const char *, ...);
extern void ApexError_argument(const char *, ...);
extern void ApexError_reference(const char *, ...);
extern void ApexError_print(ApexErrorHandler *);
extern void ApexError_parse(const char *fmt, ...);
extern void ApexError_compiler(const char *fmt, ...);

#endif 
