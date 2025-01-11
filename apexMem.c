#include <stdlib.h>
#include "apexErr.h"

/**
 * Allocate memory of the given size, or abort if out of memory.
 *
 * @param size The number of bytes to allocate.
 *
 * @return A pointer to the newly allocated memory.
 */
void *apexMem_alloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        apexErr_mem("failed to allocate %d bytes", size);
        exit(EXIT_FAILURE);
    }
    return p;
}

/**
 * Allocate an array of the given size, or abort if out of memory.
 *
 * This function is the same as apexMem_alloc, but the memory is zeroed
 * after allocation.
 *
 * @param count The number of elements in the array.
 * @param size The size of each element in the array.
 *
 * @return A pointer to the newly allocated array.
 */
void *apexMem_calloc(size_t count, size_t size) {
    void *p = calloc(count, size);
    if (!p) {
        apexErr_mem("failed to allocate %d bytes", size);
        exit(EXIT_FAILURE);
    }
    return p;
}

/**
 * Reallocate memory, or abort if out of memory.
 *
 * @param p The pointer to reallocate.
 * @param size The new size of the allocation.
 *
 * @return A pointer to the newly allocated memory.
 */
void *apexMem_realloc(void *p, size_t size) {
    void *q = realloc(p, size);
    if (!q) {
        apexErr_mem("failed to reallocate %d bytes", size);
        exit(EXIT_FAILURE);
    }
    return q;
}