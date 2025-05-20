#include "fmanage.h"
#include "salloc.h"
#include <errno.h>
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
    // Close and free all actual fm_FILE entries
    fm_FILE *current = fm->head->next;
    while (current != fm->tail) {
        fm_FILE *to_free = current;
        current = current->next;
        if (to_free->fd != NULL) {
            fclose(to_free->fd);
            // No need to adjust fm->count here as we are dismantling everything
        }
        // Assuming filename is managed externally and not sstrdup'd by fmanage itself
        free(to_free);
    }

    // Free the sentinel nodes
    free(fm->head);
    free(fm->tail);
    fm->head = NULL; // Avoid dangling pointers
    fm->tail = NULL;
}

void
fm_reopen(fmanage *fm, fm_FILE *ff) {
    FILE *fd = NULL;
    while (fm->count >= fm->limit) {
        fm_temp_close_count(fm, fm->limit - fm->count + 1);
    }

    do {
        fd = fopen(ff->filename, "r");
        ff->_errno = errno;
        if (fd == NULL && errno == EMFILE) {
            fm_temp_close_count(fm, CLOSE_FILES_COUNT);
        } else if (fd == NULL) {
            return; // check the error outside
        }
    } while(fd == NULL && fm->count > 0);

    if (fd == NULL) {
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
            errno = 0; // Clear errno before calling a function that might set it
            FILE *fd = fopen(filename, "r");
            if (fd != NULL) {
                ff = (fm_FILE *) salloc(sizeof(fm_FILE),
                                        handle_exit);
                ff->filename = filename;
                ff->pos = 0;
                ff->fd = fd;
                ff->_errno = 0; // CRITICAL: Set to 0 on successful open

                fm->head->next->prev = ff;
                ff->next = fm->head->next;
                fm->head->next = ff;
                ff->prev = fm->head;
                fm->count++;
            } else {
                // fopen failed, errno is now set by fopen
                ff = NULL; // Ensure ff remains NULL for the loop condition
                if (errno != EMFILE) {
                    // For other errors (e.g. ENOENT), return NULL immediately to signal to caller
                    // The caller (compare_files) will then use the current errno value.
                    return NULL;    
                }
                // If EMFILE, loop will continue to try closing other files
            }
        }
        if (ff == NULL) {
            fm_temp_close_count(fm, CLOSE_FILES_COUNT);
        }
    }

    return ff;
}

size_t
fm_fread(fmanage *fm, void *ptr, size_t size, size_t nmemb, fm_FILE *ff) {
    if (ff != NULL) {
        if (ff->_errno != 0 && ff->_errno != ENOENT) { // Allow reading attempt if only ENOENT was set by a previous failed open that was ignored by ufsorter
                                                 // ENOENT on ff might mean it was never successfully opened in the first place.
                                                 // A better approach might be to not even attempt fread if ff->fd is NULL from a failed fopen.
                                                 // Let's assume for now ff->_errno != 0 is a hard error for reading.
             if (ff->_errno != 0) return 0; // If ff carries an error from open/previous read, don't proceed.
        }

        if (ff->fd == NULL) {
            // File might be temporarily closed or never successfully opened.
            // If never opened (e.g. fm_fopen returned NULL and caller still tried to use it),
            // or if fm_reopen fails, this will be an issue.
            // The current design expects fm_reopen to handle it.
            errno = 0; // Clear errno before reopen
            fm_reopen(fm, ff);
            if (ff->fd == NULL) { // Reopen failed
                // ff->_errno should have been set by fm_reopen if it failed to open the file.
                // If it wasn't (e.g. fm_reopen exited), then this is problematic.
                // Assuming fm_reopen sets ff->_errno if it returns with ff->fd == NULL
                // fprintf(stderr, "Error reopening %s: %s\n", ff->filename, strerror(ff->_errno ? ff->_errno : errno));
                return 0; // Indicate read failure
            }
        }

        errno = 0; // Clear errno before calling fread
        size_t cnt = fread(ptr, size, nmemb, ff->fd);
        // After fread, errno is set ONLY IF an error occurred. 
        // If EOF, feof(ff->fd) is true. If error, ferror(ff->fd) is true.

        // Store errno only if a read error actually occurred.
        // fread returns a short count on EOF or error.
        if (cnt < nmemb && ferror(ff->fd)) {
            ff->_errno = errno; // Store actual read error
        } else {
            // No read error, or just EOF. Don't overwrite a previous ff->_errno from open phase
            // unless explicitly clearing it. If open was successful, ff->_errno was 0.
            // If it was EOF, ff->_errno should remain 0 for this path.
            if (ff->_errno == 0) { // If no prior error on this fm_FILE struct
                 // do not set ff->_errno from global errno here if it was just EOF
            } 
        }
        // The old code: ff->_errno = errno; was too broad.
        // It should be: if (ferror(ff->fd)) ff->_errno = errno; else if successful open ff->_errno = 0 for read;

        fm->total_readed += cnt; // This seems to be a global counter in fmanage, its utility isn't clear here.
        ff->pos += cnt;

        // LRU cache update
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
