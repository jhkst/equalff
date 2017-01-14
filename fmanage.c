#include "fmanage.h"
#include "salloc.h"
#include "salloc.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define CLOSE_FILES_COUNT 1

void
fm_init(fmanage *fm, int limit) {
    fm->count = 0;
    fm->limit = limit;
    fm->head = (fm_FILE *) salloc(sizeof(fm_FILE), handle_exit);
    fm->tail = (fm_FILE *) salloc(sizeof(fm_FILE), handle_exit);
    fm->head->next = fm->tail;
    fm->head->prev = NULL;
    fm->tail->next = NULL;
    fm->tail->prev = fm->head;
}

void
fm_temp_close_file(fmanage *fm, fm_FILE *ff) {
    if (ff->fd != NULL) {
        fclose(ff->fd);
        ff->_errno = errno;
        ff->fd = NULL;

        fm->count--;

        ff->prev->next = ff->next;
        ff->next->prev = ff->prev;
    }
}

void
fm_temp_close_count(fmanage *fm, int count) {
    for (int i = 0; i < count; i++) {
        if (fm->count > 0) {
            fm_temp_close_file(fm, fm->tail->prev);
        }
    }
}

void
fm_free(fmanage *fm) {
    free(fm->head);
    free(fm->tail);
}

void
fm_reopen(fmanage *fm, fm_FILE *ff) {
    FILE *fd = NULL;
    while (fm->count >= fm->limit) {
        fm_temp_close_count(fm, fm->limit - fm->count + 1);
    }

    while (fd == NULL && fm->count > 0) {
        fd = fopen(ff->filename, "r");
        ff->_errno = errno;
        if (fd == NULL && errno == EMFILE) {
            fm_temp_close_count(fm, CLOSE_FILES_COUNT);
        } else if (fd == NULL) {
            return; // check the error outside
        }
    }

    if (fm->count <= 0 || fd == NULL) {
        fprintf(stderr, "Failed to allocate file open: %s\n", ff->filename);
        exit(1);
    }

    ff->fd = fd;

    fm->head->next->prev = ff;
    ff->next = fm->head->next;
    fm->head->next = ff;
    ff->prev = fm->head;
    fm->count++;

    fseek(fd, ff->pos, SEEK_SET);
}

fm_FILE *
fm_fopen(fmanage *fm, char *filename) {
    fm_FILE *ff = NULL;

    while (ff == NULL) {
        if (fm->count < fm->limit) {
            FILE *fd = fopen(filename, "r");
            if (fd != NULL) {
                ff = (fm_FILE *) salloc(sizeof(fm_FILE),
                                        handle_exit);
                ff->filename = filename;
                ff->pos = 0;
                ff->fd = fd;
                ff->_errno = errno;

                fm->head->next->prev = ff;
                ff->next = fm->head->next;
                fm->head->next = ff;
                ff->prev = fm->head;
                fm->count++;
            } else {
                if (errno != EMFILE) {
                    return NULL;    // check the error outside
                }
            }
        }
        if (ff == NULL) {
            fm_temp_close_count(fm, CLOSE_FILES_COUNT);
        }
    }

    return ff;
}

int
fm_fread(fmanage *fm, void *ptr, size_t size, size_t nmemb, fm_FILE *ff) {
    if (ff != NULL) {
        if (ff->_errno != 0) {
            return 0;
        }
        if (ff->fd == NULL) {
            fm_reopen(fm, ff);
            if (ff->fd == NULL) {
                ff->_errno = errno;
                fprintf(stderr, "Error opening %s: %s", ff->filename,
                        strerror(errno));
                return 0;
            }
        }

        int cnt = fread(ptr, size, nmemb, ff->fd);
        fm->total_readed += cnt;

        ff->pos += cnt;
        ff->_errno = errno;

        ff->prev->next = ff->next;
        ff->next->prev = ff->prev;
        fm->head->next->prev = ff;
        ff->next = fm->head->next;
        fm->head->next = ff;
        ff->prev = fm->head;

        return cnt;
    }
    return 0;
}

void
fm_fclose(fmanage *fm, fm_FILE *ff) {
    fm_temp_close_file(fm, ff);
    ff->fd = NULL;
    ff->filename = NULL;
    ff->next = NULL;
    ff->prev = NULL;
    ff->pos = -1;
    ff->_errno = -1;
    free(ff);
}
