#ifndef _CMPDATA_H
#define _CMPDATA_H

#include "fmanage.h"
#include "salloc.h"
#include <stdio.h>

#define BUFFER_SIZE 4096

typedef struct cmpdata {
    int size;
    fm_FILE **file;
    int *order;
    int *uf_parent;
    char **data;
    long int pos;           // TODO:
    int readed;
    int buffer_size;
} cmpdata;

void cmp_init(cmpdata *cd, int size, int max_buffer);

int cmp_uf_ordered_same(cmpdata *cd, int sidx1, int sidx2);

int cmp_uf_same(cmpdata *cd, int idx1, int idx2);

int cmp_uf_root(cmpdata *cd, int idx);

void cmp_uf_reset_ordered(cmpdata *cd, int sidx, int size);

void cmp_uf_diff(cmpdata *cd, int idx1, int idx2);

void cmp_uf_union(cmpdata *cd, int idx1, int idx2);

void cmp_free(cmpdata *cd);

#endif
