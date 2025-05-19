#ifndef _SALLOC_H
#define _SALLOC_H

#include <stdlib.h>
#include <string.h>

void handle_exit();

void *salloc(size_t size, void (*error_handle)());
char *sstrdup(const char *s, void (*error_handle)());

#endif
