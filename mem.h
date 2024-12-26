#ifndef MEM_H
#define MEM_H

extern void *mem_alloc(size_t size);
extern void *mem_calloc(size_t count, size_t size);
extern void *mem_realloc(void *p, size_t size);

#endif