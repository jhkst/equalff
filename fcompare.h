#ifndef _FCOMPARE_H
#define _FCOMPARE_H

#include <stdio.h>

#define MAX_OPEN_FILES FOPEN_MAX

extern size_t total_bytes_read;

int             compare_files(char *name[], int count, int max_buffer);

#endif
