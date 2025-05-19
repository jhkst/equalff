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

/**
 * Safe strdup
 * @param s string to duplicate
 * @param error_handle function to call on error
 * @return pointer to allocated duplicated string
 */
char *
sstrdup(const char *s, void (*error_handle)()) {
    char *new_s = strdup(s);
    if (new_s == NULL) {
        error_handle();
    }
    return new_s;
}
