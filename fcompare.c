#include "cmpdata.h"
#include "fcompare.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int
#ifdef __APPLE__
ufsorter(void *arg, const void *p1, const void *p2)
#else
ufsorter(const void *p1, const void *p2, void *arg)
#endif
{
    int f1 = *((int *) p1);
    int f2 = *((int *) p2);
    cmpdata *cd = (cmpdata *) arg;

    int cmp = memcmp(cd->data[f1], cd->data[f2], cd->readed);

    if (cmp == 0) {
        cmp_uf_union(cd, f1, f2);
    } else {
        cmp_uf_diff(cd, f1, f2);
    }

    return cmp;
}

int
compare_files(char *name[], int count, int max_buffer, int max_open_files) {
    if (count < 2) {
        return 0;
    }

    fmanage *fm = (fmanage *) salloc(sizeof(fmanage), handle_exit);
    fm_init(fm, max_open_files > 0 ? max_open_files : count);

    cmpdata *cmp_data = (cmpdata *) salloc(sizeof(cmpdata), handle_exit);

    cmp_init(cmp_data, count, max_buffer);

    size_t readed;
    int group_start;
    size_t group_size;
    int idx = 0;

    int *order_cpy = (int *) salloc(sizeof(int) * count, handle_exit);

    do {
        group_start = idx;
        group_size = 0;
        readed = 0;

        while (idx < count
               && cmp_uf_ordered_same(cmp_data, group_start, idx)) {
            idx++;
            group_size++;
        }

        if (group_size > 1) {
            for (int i = group_start; i < group_size + group_start; i++) {
                int realidx = cmp_data->order[i];
                if (cmp_data->file[realidx] == NULL) {
                    cmp_data->file[realidx] =
                            fm_fopen(fm, name[realidx]);
                    if (cmp_data->file[realidx] == NULL) {
                        fprintf(stderr,
                                "Cannot open file '%s': %s\n",
                                name[realidx],
                                strerror(errno));
                    }
                }
                readed =
                        fm_fread(fm, cmp_data->data[realidx], 1,
                                 cmp_data->buffer_size,
                                 cmp_data->file[realidx]);
            }
            cmp_uf_reset_ordered(cmp_data, group_start, group_size);
            cmp_data->readed = readed;

            memcpy(order_cpy, cmp_data->order, sizeof(int) * count);
            #ifdef __APPLE__
            qsort_r(&order_cpy[group_start], group_size, sizeof(int), cmp_data, ufsorter);
            #else
            qsort_r(&order_cpy[group_start], group_size, sizeof(int), ufsorter, cmp_data);
            #endif
            memcpy(cmp_data->order, order_cpy, sizeof(int) * count);
        } else {
            if (cmp_data->file[cmp_data->order[group_start]] != NULL) {
                fm_fclose(fm,
                          cmp_data->file[cmp_data->
                                  order[group_start]]);
                cmp_data->file[cmp_data->order[group_start]] = NULL;
            }
        }

        if (idx >= count) {
            idx = 0;
        }
    } while (readed > 0);

    free(order_cpy);

    for (int i = 0; i < count; i++) {
        if (cmp_data->file[i] != NULL) {
            fm_fclose(fm, cmp_data->file[i]);
        }
    }

    fm_free(fm);
    free(fm);

    int cluster_count = 0;
    char *prev = name[cmp_data->order[0]];
    int more_than_one = 0;
    for (int i = 1; i < count + 1; i++) {
        if (i < count && cmp_uf_ordered_same(cmp_data, i - 1, i)) {
            more_than_one = 1;
            fprintf(stdout, "%s\n", prev);
        } else {
            if (more_than_one) {
                fprintf(stdout, "%s\n\n", prev);
                cluster_count++;
            }
            more_than_one = 0;
        }
        if (i < count) {
            prev = name[cmp_data->order[i]];
        }
    }

    cmp_free(cmp_data);
    free(cmp_data);

    return cluster_count;
}
