#include "salloc.h"

#include <stdio.h>
#include <stdlib.h>

void
handle_exit() {
    fprintf(stderr, "\nOut of memory\n");
}

void *
salloc(size_t size, void (*error_handle)()) {
    void *ref = malloc(size);
    if (ref == NULL) {
        error_handle();
    }
    return ref;
}
