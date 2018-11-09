#ifndef APEX_MALLOC_H
#define APEX_MALLOC_H

#include <stddef.h>

extern void *apex_malloc(size_t);
extern void *apex_calloc(size_t, size_t);
extern void *apex_realloc(void *, size_t);
extern void apex_free(void *);

#define APEX_NEW(T) apex_malloc(sizeof(T))

#endif 
