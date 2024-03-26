#include "salloc.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Handle exit on error
 */
void
handle_exit() {
    fprintf(stderr, "\nOut of memory\n");
    exit(10);
}

/**
 * Safe malloc
 * @param size size of memory to allocate
 * @param error_handle function to call on error
 * @return pointer to allocated memory
 */
void *
salloc(size_t size, void (*error_handle)()) {
    void *ref = malloc(size);
    if (ref == NULL) {
        error_handle();
    }
    return ref;
}
