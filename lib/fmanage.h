#ifndef _FMANAGE_H
#define _FMANAGE_H

#include <stdio.h>

typedef struct fm_FILE {
    char *filename;
    long int pos;
    FILE *fd;
    int _errno;

    struct fm_FILE *prev;
    struct fm_FILE *next;

} fm_FILE;

typedef struct fmanage {
    int count;
    int limit;
    fm_FILE *head;
    fm_FILE *tail;
    size_t total_readed;
} fmanage;

void fm_init(fmanage *fm, int limit);

fm_FILE *fm_fopen(fmanage *fm, char *filename);

size_t fm_fread(fmanage *fm, void *ptr, size_t size, size_t nmemb,
             fm_FILE *stream);

void fm_fclose(fmanage *fm, fm_FILE *ff);

void fm_free(fmanage *fm);

#endif