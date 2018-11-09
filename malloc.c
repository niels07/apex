#include <stdlib.h>
#include <stdio.h>

#include "error.h"

static void out_of_memory(size_t size) {
    apex_error_throw(APEX_ERROR_CODE_NOMEM, "failed to allocate %d bytes", size);       
}

void *apex_malloc(size_t size) {
    void *p = malloc(size);

    if (!p) {
        out_of_memory(size);
    }
    return p;
}

void *apex_calloc(size_t n, size_t size) {
    void *p = calloc(n, size);

    if (!p) {
        out_of_memory(size);
    }
    return p;
}

void *apex_realloc(void *p, size_t size) {
    void *newP;

    if (!p) {
        return apex_malloc(size);
    }

    newP = realloc(p, size);
    if (!newP) {
        out_of_memory(size);
    }
    return p;
}

void apex_free(void *p) {
    if (p) {
        free(p);
    }
}
