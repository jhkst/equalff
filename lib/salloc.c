#include "salloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * @param error_handle function to call on error. If NULL and allocation fails, returns NULL.
 * @return pointer to allocated memory, or NULL if allocation fails and error_handle is NULL.
 */
void *
salloc(size_t size, void (*error_handle)()) {
    if (size == 0) {
    }
    void *ref = malloc(size);
    if (ref == NULL) {
        if (error_handle != NULL) {
            error_handle();
        }
        return NULL;
    }
    return ref;
}

/**
 * Safe strdup
 * @param s string to duplicate
 * @param error_handle function to call on error. If NULL and allocation fails, returns NULL.
 * @return pointer to allocated duplicated string, or NULL if allocation fails and error_handle is NULL.
 */
char *
sstrdup(const char *s, void (*error_handle)()) {
    if (s == NULL) {
        if (error_handle != NULL) {
        }
        return NULL;
    }

    char *new_s = strdup(s);

    if (new_s == NULL) {
        if (error_handle != NULL) {
            error_handle();
        }
        return NULL;
    }
    return new_s;
}
