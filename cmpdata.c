#include "cmpdata.h"

#define MIN_BUFFER_PER_FILE 128

void
cmp_init(cmpdata *cd, int size, size_t max_buffer) {
    cd->size = size;
    cd->order = (int *) salloc(sizeof(int) * size, handle_exit);
    cd->uf_parent = (int *) salloc(sizeof(int) * size, handle_exit);
    cd->file = (fm_FILE **) salloc(sizeof(FILE *) * size, handle_exit);
    cd->data = (char **) salloc(sizeof(char *) * size, handle_exit);

    if (max_buffer >= 0 && max_buffer / size < MIN_BUFFER_PER_FILE) {
        fprintf(stderr,
                "Minimal buffer size is too small. At least %d per one compared (same file size) files\n",
                MIN_BUFFER_PER_FILE);
        exit(1);
    }

    cd->buffer_size = (max_buffer <= 0
                       || max_buffer / size >
                          BUFFER_SIZE) ? BUFFER_SIZE : max_buffer / size;
    if (cd->buffer_size < MIN_BUFFER_PER_FILE) {
        cd->buffer_size = MIN_BUFFER_PER_FILE;
    }

    for (int i = 0; i < size; i++) {
        cd->data[i] =
                (char *) salloc(sizeof(char) * cd->buffer_size, handle_exit);
        cd->order[i] = i;
        cd->file[i] = NULL;
        cd->uf_parent[i] = 0;
    }
}

void
cmp_free(cmpdata *cd) {
    for (int i = 0; i < cd->size; i++) {
        free(cd->data[i]);
    }
    free(cd->data);
    free(cd->file);
    free(cd->uf_parent);
    free(cd->order);
}

int
cmp_uf_ordered_same(cmpdata *cd, int sidx1, int sidx2) {
    return cmp_uf_same(cd, cd->order[sidx1], cd->order[sidx2]);
}

int
cmp_uf_same(cmpdata *cd, int idx1, int idx2) {
    return cmp_uf_root(cd, idx1) == cmp_uf_root(cd, idx2);
}

int
cmp_uf_root(cmpdata *cd, int idx) {
    if (cd->uf_parent[idx] < 0) {
        return cd->uf_parent[idx];
    }
    while (idx != cd->uf_parent[idx]) {
        cd->uf_parent[idx] = cd->uf_parent[cd->uf_parent[idx]];
        idx = cd->uf_parent[idx];
    }
    return idx;
}

void
cmp_uf_reset_ordered(cmpdata *cd, int sidx, size_t size) {
    size_t last = sidx + size;
    for (int i = sidx; i < last; i++) {
        cd->uf_parent[cd->order[i]] = -1;
    }
}

void
cmp_uf_union(cmpdata *cd, int idx1, int idx2) {
    int not_set1 = cd->uf_parent[idx1] < 0;
    int not_set2 = cd->uf_parent[idx2] < 0;
    if (not_set1 && not_set2) {
        cd->uf_parent[idx1] = cd->uf_parent[idx2] = idx1;
    } else if (not_set1) {
        cd->uf_parent[idx1] = idx2;
    } else if (not_set2) {
        cd->uf_parent[idx2] = idx1;
    } else {
        int root1 = cmp_uf_root(cd, idx1);
        int root2 = cmp_uf_root(cd, idx2);
        if (root1 != root2) {
            cd->uf_parent[root2] = root1;
        }
    }
}

void
cmp_uf_diff(cmpdata *cd, int idx1, int idx2) {
    if (cd->uf_parent[idx1] < 0) {
        cd->uf_parent[idx1] = idx1;
    }
    if (cd->uf_parent[idx2] < 0) {
        cd->uf_parent[idx2] = idx2;
    }
}
