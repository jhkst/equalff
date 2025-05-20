#include "cmpdata.h"
#include <errno.h> // For ENOMEM, EINVAL

#define MIN_BUFFER_PER_FILE 128

/**
 * Initialize cmpdata structure.
 * @param cd cmpdata structure to initialize.
 * @param size number of files to manage.
 * @param max_buffer maximal total buffer size for all files.
 * @return 0 on success, non-zero (e.g., ENOMEM or EINVAL) on failure.
 *         On failure, any partially allocated members within cd should be freed by a call to cmp_free.
 */
int
cmp_init(cmpdata *cd, int size, size_t max_buffer) {
    // Initialize pointers to NULL so cmp_free can be called safely on partial failure.
    cd->order = NULL;
    cd->uf_parent = NULL;
    cd->file = NULL;
    cd->data = NULL;
    cd->size = 0; // Set size to 0 initially
    cd->readed = 0;
    cd->buffer_size = 0;

    if (size <= 0) { // Cannot handle non-positive size
        return EINVAL;
    }

    cd->size = size; // Set actual size now

    // Use NULL as the error handler for salloc, so it returns NULL on failure.
    cd->order = (int *) salloc(sizeof(int) * size, NULL);
    if (!cd->order) return ENOMEM;

    cd->uf_parent = (int *) salloc(sizeof(int) * size, NULL);
    if (!cd->uf_parent) return ENOMEM; // cmp_free will handle already allocated cd->order

    cd->file = (fm_FILE **) salloc(sizeof(fm_FILE *) * size, NULL);
    if (!cd->file) return ENOMEM; // cmp_free handles previous allocations

    cd->data = (char **) salloc(sizeof(char *) * size, NULL);
    if (!cd->data) return ENOMEM; // cmp_free handles previous allocations

    // Initialize all data pointers to NULL before allocation loop
    for (int i = 0; i < size; i++) {
        cd->data[i] = NULL;
    }

    if (max_buffer > 0 && (size_t)size > 0 && max_buffer / (size_t)size < MIN_BUFFER_PER_FILE) {
        // fprintf(stderr, // Library should not print to stderr for this kind of validation error.
        //         "Minimal buffer size is too small. At least %d per one compared (same file size) files\n",
        // MIN_BUFFER_PER_FILE);
        // exit(1); // Replaced with error return
        return EINVAL; // Invalid argument for buffer size
    }

    cd->buffer_size = (max_buffer == 0 || ( (size_t)size > 0 && max_buffer / (size_t)size > BUFFER_SIZE) ) 
                      ? BUFFER_SIZE 
                      : max_buffer / (size_t)size;
    if (cd->buffer_size < MIN_BUFFER_PER_FILE) {
        cd->buffer_size = MIN_BUFFER_PER_FILE;
    }
    if (size > 0 && cd->buffer_size == 0) { // Avoid zero buffer size if size > 0
        cd->buffer_size = MIN_BUFFER_PER_FILE; 
    }


    for (int i = 0; i < size; i++) {
        cd->data[i] = (char *) salloc(sizeof(char) * cd->buffer_size, NULL);
        if (!cd->data[i]) {
            // If one allocation fails, cmp_free will clean up previously allocated cd->data[0...i-1]
            // and other main arrays (cd->order, cd->uf_parent, etc.)
            return ENOMEM;
        }
        cd->order[i] = i;
        cd->file[i] = NULL;
        cd->uf_parent[i] = 0; // Or -1 for root convention, if that's chosen elsewhere.
                              // Current logic seems to use 0 and then set to self-index or parent index.
                              // The cmp_uf_root and union logic should be consistent.
                              // Initializing to 0 and then setting parent[idx1]=parent[idx2]=idx1 on first union is one way.
                              // A common union-find init is parent[i] = i or parent[i] = -1 (size).
                              // The existing cmp_uf_root handles uf_parent[idx] < 0 as a special case.
                              // Let's ensure the reset/union logic matches. Current uf_reset_ordered uses -1.
                              // It might be better to initialize uf_parent[i] = -1 here for consistency with reset.
        cd->uf_parent[i] = -1; // Initialize as root of its own set with size 1 (convention for negative values)

    }
    return 0; // Success
}

/**
 * Free cmpdata structure
 * @param cd cmpdata structure
 */
void
cmp_free(cmpdata *cd) {
    if (!cd) return;

    if (cd->data) {
        // cd->size would be 0 if initial allocations for order/uf_parent/file/data failed before size was set.
        // If cd->size is > 0, it means the main arrays were allocated.
        // The loop for cd->data[i] should use the actual cd->size that was set during successful part of init.
        for (int i = 0; i < cd->size; i++) {
            if (cd->data[i]) { // Check if individual buffer was allocated
                free(cd->data[i]);
            }
        }
        free(cd->data);
    }
    if (cd->file) free(cd->file);
    if (cd->uf_parent) free(cd->uf_parent);
    if (cd->order) free(cd->order);
    // Reset fields to prevent accidental use after free, though cd itself is usually freed by caller after this.
    cd->data = NULL;
    cd->file = NULL;
    cd->uf_parent = NULL;
    cd->order = NULL;
    cd->size = 0;
}

/**
 * Compare two files
 * @param cd cmpdata structure
 * @param idx1 index of first file
 * @param idx2 index of second file
 * @return 0 if files are same, 1 if files are different, -1 if error
 */
int
cmp_uf_ordered_same(cmpdata *cd, int sidx1, int sidx2) {
    return cmp_uf_same(cd, cd->order[sidx1], cd->order[sidx2]);
}

int
cmp_uf_same(cmpdata *cd, int idx1, int idx2) {
    return cmp_uf_root(cd, idx1) == cmp_uf_root(cd, idx2);
}

/**
 * Find root of union-find structure
 * @param cd cmpdata structure
 * @param idx index of file
 * @return root of union-find structure
 */
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

/**
 * Union two files
 * @param cd cmpdata structure
 * @param idx1 index of first file
 * @param idx2 index of second file
 */
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