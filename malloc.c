#include <stdlib.h>
#include <stdio.h>

#include "error.h"

static void out_of_memory(size_t size) {
    ApexError_throw(APEX_ERROR_CODE_MEMORY,
        "failed to allocate %d bytes", size);       
}

void *apex_malloc(size_t size) {
    void *ptr = malloc(size);

    if (!ptr) {
        out_of_memory(size);
    }
    return ptr;
}

void *apex_calloc(size_t n, size_t size) {
    void *ptr = calloc(n, size);

    if (!ptr) {
        out_of_memory(size);
    }
    return ptr;
}

void *apex_realloc(void *ptr, size_t size) {
    void *new_ptr;

    if (!ptr) {
        return apex_malloc(size);
    }

    new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        out_of_memory(size);
    }
    return new_ptr;
}

char *dupstr(const char *src) {
    char *str;
    char *ptr;
    int len = 0;

    while (src[len]) {
        len++;
    }
    str = apex_malloc(len + 1);
    ptr = str;
    while (*src) {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return str;
}

void apex_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}
