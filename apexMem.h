#ifndef MEM_H
#define MEM_H

#include <stdio.h>

extern void *apexMem_alloc(size_t size);
extern void *apexMem_calloc(size_t count, size_t size);
extern void *apexMem_realloc(void *p, size_t size);

#endif