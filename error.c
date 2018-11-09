#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"


struct ApexErrorHandler {
    ApexErrorCallback *callback;
    void *data;
    char message[APEX_ERROR_MAX];
    ApexErrorCode error_code;
};

static const char *ERROR_MESSAGES[] = {
    "internal",
    "IO",
    "out of memory",
    "syntax",
    "type",
    "runtime",
    "argument"
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

void apex_error_set_handler(ApexErrorCode error_code, ApexErrorCallback *callback, void *data) {
    ApexErrorHandler *handler;

    init_handlers();
    handler = handlers + error_code;
    handler->callback = callback;
    handler->data = data;
}

void apex_error_print(ApexErrorHandler *handler) {
    const char *errdesc = ERROR_MESSAGES[handler->error_code];

    fprintf(stderr, "apex: %s error: %s\n", errdesc, handler->message);
}

void apex_error_vthrow(ApexErrorCode error_code, const char *fmt, va_list vl) {
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
    apex_error_vthrow(error_code, fmt, vl); \
} while (0)

void apex_error_throw(ApexErrorCode error_code, const char *fmt, ...) {
    throw_error(error_code, fmt);
}

void apex_error_io(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_IO, fmt);
}

void apex_error_internal(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_INTERNAL, fmt);
}

void apex_error_runtime(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_RUNTIME, fmt);
}

void apex_error_syntax(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_SYNTAX, fmt);
}

void apex_error_type(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_TYPE, fmt);
}

void apex_error_nomem(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_NOMEM, fmt);
}

void apex_error_argument(const char *fmt, ...) {
    throw_error(APEX_ERROR_CODE_ARGUMENT, fmt);
}
