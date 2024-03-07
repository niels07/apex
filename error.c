#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


struct ApexErrorHandler {
    ApexErrorCallback *callback;
    void *data;
    char message[ApexError_MAX];
    ApexErrorCode error_code;
};

static const char *ERROR_MESSAGES[] = {
    "internal",
    "IO",
    "memory allocation",
    "syntax",
    "type",
    "runtime",
    "argument",
    "compiler",
    "out of bounds"
};

static ApexErrorHandler handlers[64];
static int initialized = 0;
    
static void init_handlers(void) {
    if (initialized) {
        return;
    }
    memset(handlers, 0, 65 * sizeof(ApexErrorHandler));
    initialized = 1;
}

void ApexError_set_handler(ApexErrorCode error_code, ApexErrorCallback *callback, void *data) {
    ApexErrorHandler *handler;

    init_handlers();
    handler = handlers + error_code;
    handler->callback = callback;
    handler->data = data;
}

void ApexError_print(ApexErrorHandler *handler) {
    const char *errdesc = ERROR_MESSAGES[handler->error_code];

    fprintf(stderr, "apex: %s error: %s\n", errdesc, handler->message);
}

void ApexError_vthrow(ApexErrorCode error_code, const char *fmt, va_list vl) {
    ApexErrorHandler *handler;

    init_handlers();
    handler = handlers + error_code;
    vsprintf(handler->message, fmt, vl);
    va_end(vl);

    if (!handler->callback) {
        const char *errdesc = ERROR_MESSAGES[error_code];
        fprintf(stderr, "apex: %s error: %s\n", errdesc, handler->message);
        exit(EXIT_FAILURE);
    }
    handler->error_code = error_code;
    handler->callback(handler);
}

#define throw_error(error_code, fmt) do {   \
    va_list vl;                             \
    va_start(vl, fmt);                      \
    ApexError_vthrow(error_code, fmt, vl); \
} while (0)

void ApexError_throw(ApexErrorCode error_code, const char *fmt, ...) {
    throw_error(error_code, fmt);
}

void ApexError_io(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_IO, fmt);
}

void ApexError_internal(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_INTERNAL, fmt);
}

void ApexError_runtime(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_RUNTIME, fmt);
}

void ApexError_syntax(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_SYNTAX, fmt);
}

void ApexError_type(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_TYPE, fmt);
}

void ApexError_out_of_bounds(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_OUT_OF_BOUNDS, fmt);
}

void ApexError_argument(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_ARGUMENT, fmt);
}

void ApexError_reference(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_REFERENCE, fmt);
}

void ApexError_parse(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_REFERENCE, fmt);
}

void ApexError_compiler(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_COMPILER, fmt);
}