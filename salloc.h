#ifndef _SALLOC_H
#define _SALLOC_H

#include <stdlib.h>

void            handle_exit();

void           *salloc(size_t size, void (*error_handle)());

#endif
