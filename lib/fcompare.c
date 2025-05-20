#include "cmpdata.h"
#include "fcompare.h"
#include "salloc.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h> // For SIZE_MAX if needed, or use a large number

// Context structure for the adapter - MOVED BEFORE CALLBACK
struct CompareFilesAsyncAdapterContext {
    ComparisonResult *result;
};

// Static callback for compare_files to adapt compare_files_async
static void compare_files_adapter_callback(const DuplicateSet *duplicates_from_async, void *user_data) {
    struct CompareFilesAsyncAdapterContext *adapter_ctx = (struct CompareFilesAsyncAdapterContext *)user_data;
    ComparisonResult *result = adapter_ctx->result;

    if (result->error_code == ENOMEM && result->error_message != NULL) {
        return;
    }

    DuplicateSet *new_sets_ptr = (DuplicateSet *)realloc(result->sets, (result->count + 1) * sizeof(DuplicateSet));
    if (!new_sets_ptr) {
        if (result->error_message == NULL) { 
            result->error_message = sstrdup("Adapter: Failed to reallocate memory for duplicate sets array.", NULL);
            // If sstrdup itself fails for the error message, we can't do much more here for the message.
        }
        result->error_code = ENOMEM;
        return; 
    }
    result->sets = new_sets_ptr;
    DuplicateSet *current_target_set = &result->sets[result->count];
    current_target_set->paths = NULL; 
    current_target_set->count = 0;    

    current_target_set->paths = (char **)salloc((duplicates_from_async->count) * sizeof(char *), NULL); // Pass NULL for error_handle
    if (!current_target_set->paths) {
        if (result->error_message == NULL) { 
            result->error_message = sstrdup("Adapter: Failed to salloc paths for set.", NULL);
        }
        result->error_code = ENOMEM; 
        return;
    }

    int paths_copied = 0;
    for (int i = 0; i < duplicates_from_async->count; i++) {
        current_target_set->paths[i] = sstrdup(duplicates_from_async->paths[i], NULL); // Pass NULL for error_handle
        if (!current_target_set->paths[i]) {
            if (result->error_message == NULL) {
                 result->error_message = sstrdup("Adapter: Failed to sstrdup path string.", NULL);
            }
            result->error_code = ENOMEM;
            for (int j = 0; j < paths_copied; j++) {
                free(current_target_set->paths[j]);
            }
            free(current_target_set->paths);
            current_target_set->paths = NULL;
            return;
        }
        paths_copied++;
    }
    current_target_set->count = paths_copied;
    result->count++; 
}

// No longer needed here as it's defined above
// typedef struct CompareFilesAsyncAdapterContext CompareFilesAsyncAdapterContext;


// No longer needed, functionality moved to compare_files_adapter_callback
/*
static int add_duplicate_set(ComparisonResult *result, char *name[], const int order[], int group_start_idx_in_order, int files_in_cluster) {
    // ... original implementation ...
}
*/

int
#ifdef __APPLE__
ufsorter(void *arg, const void *p1, const void *p2)
#else
ufsorter(const void *p1, const void *p2, void *arg)
#endif
{
    int f1_idx = *((int *) p1);
    int f2_idx = *((int *) p2);
    cmpdata *cd = (cmpdata *) arg;

    if (f1_idx < 0 || f1_idx >= cd->size || f2_idx < 0 || f2_idx >= cd->size) {
        return 0; 
    }

    int f1_struct_null = (cd->file[f1_idx] == NULL);
    int f2_struct_null = (cd->file[f2_idx] == NULL);

    if (f1_struct_null && f2_struct_null) {
        cmp_uf_diff(cd, f1_idx, f2_idx); 
        return 0;
    }
    if (f1_struct_null || f2_struct_null) {
        cmp_uf_diff(cd, f1_idx, f2_idx);
        return f1_struct_null ? -1 : 1;
    }

    int f1_open_failed = (cd->file[f1_idx]->fd == NULL && cd->file[f1_idx]->_errno != 0); 
    int f2_open_failed = (cd->file[f2_idx]->fd == NULL && cd->file[f2_idx]->_errno != 0);

    if (f1_open_failed && f2_open_failed) {
        cmp_uf_diff(cd, f1_idx, f2_idx);
        return 0;
    }
    if (f1_open_failed || f2_open_failed) {
        cmp_uf_diff(cd, f1_idx, f2_idx);
        return f1_open_failed ? -1 : 1;
    }
    
    int f1_has_read_error = (cd->file[f1_idx]->_errno != 0 && (cd->file[f1_idx]->fd == NULL || !feof(cd->file[f1_idx]->fd)) );
    int f2_has_read_error = (cd->file[f2_idx]->_errno != 0 && (cd->file[f2_idx]->fd == NULL || !feof(cd->file[f2_idx]->fd)) );

    if (f1_has_read_error && f2_has_read_error) {
        cmp_uf_diff(cd, f1_idx, f2_idx);
        return 0; 
    }
    if (f1_has_read_error || f2_has_read_error) {
        cmp_uf_diff(cd, f1_idx, f2_idx);
        return f1_has_read_error ? -1 : 1; 
    }
    
    if (cd->readed == 0) { 
        cmp_uf_union(cd, f1_idx, f2_idx);
        return 0;
    }

    int cmp = memcmp(cd->data[f1_idx], cd->data[f2_idx], cd->readed);

    if (cmp == 0) {
        cmp_uf_union(cd, f1_idx, f2_idx);
    } else {
        cmp_uf_diff(cd, f1_idx, f2_idx);
    }
    return cmp;
}

ComparisonResult*
compare_files(char *name[], int count, int max_buffer, int max_open_files) {
    ComparisonResult *result = (ComparisonResult*)salloc(sizeof(ComparisonResult), NULL); // Pass NULL for error_handle
    if (!result) {
        // salloc failed for the main result structure. Cannot set error message in it.
        // Consider logging to stderr or having a global error state if this is critical.
        return NULL; 
    }

    result->sets = NULL;
    result->count = 0;
    result->error_code = 0;
    result->error_message = NULL; 

    if (count < 2) {
        return result; 
    }

    struct CompareFilesAsyncAdapterContext adapter_ctx;
    adapter_ctx.result = result;

    char *async_error_msg = NULL;
    int async_ret_code = compare_files_async(
        name,            // file_paths
        count,
        max_buffer,      // max_buffer_per_file
        max_open_files,
        compare_files_adapter_callback,
        &adapter_ctx,
        &async_error_msg
    );

    if (async_ret_code != 0) {
        if (result->error_code == 0) { 
            result->error_code = async_ret_code;
            if (async_error_msg) {
                if (result->error_message == NULL) {
                    result->error_message = async_error_msg; // Transfer ownership
                } else {
                    free_error_message(async_error_msg); // Result already has an error, discard this one
                }
            } else if (result->error_message == NULL) {
                char generic_msg_buf[128];
                snprintf(generic_msg_buf, sizeof(generic_msg_buf), "compare_files_async reported error %d without specific message.", async_ret_code);
                result->error_message = sstrdup(generic_msg_buf, NULL);
            }
        } else {
             if (async_error_msg) free_error_message(async_error_msg);
        }
    } else {
        if (async_error_msg) {
             // compare_files_async returned 0 (success) but also an error message.
             // This is an inconsistent state from compare_files_async.
             // If result has no error, we might log/use async_error_msg, or just free it.
             // For now, if result is fine, free the unexpected message.
            if(result->error_code == 0 && result->error_message == NULL) {
                // Log this unexpected message if logging is available
                // fprintf(stderr, "Warning: compare_files_async succeeded but returned message: %s\n", async_error_msg);
            }
            free_error_message(async_error_msg);
        }
    }

    return result;
}


void free_comparison_result(ComparisonResult *result) {
    if (!result) {
        return;
    }
    if (result->sets) {
        for (int i = 0; i < result->count; i++) {
            if (result->sets[i].paths) {
                for (int j = 0; j < result->sets[i].count; j++) {
                    if (result->sets[i].paths[j]) { 
                        free(result->sets[i].paths[j]);
                    }
                }
                free(result->sets[i].paths);
            }
        }
        free(result->sets);
    }
    if (result->error_message) {
        free(result->error_message);
    }
    free(result);
}

// New function implementations

void free_error_message(char *error_message) {
    if (error_message) {
        free(error_message);
    }
}

int compare_files_async(
    char *file_paths[],
    int count,
    int max_buffer_per_file,
    int max_open_files,
    DuplicateFoundCallback callback,
    void *user_data,
    char **error_message_out) {

    if (error_message_out) {
        *error_message_out = NULL;
    }

    if (count < 0 || (count > 0 && file_paths == NULL) || max_buffer_per_file <= 0 || callback == NULL) {
        if (error_message_out) {
            *error_message_out = sstrdup("Invalid arguments (NULL file_paths, count < 0, zero/negative max_buffer, or NULL callback).", NULL);
            if (*error_message_out == NULL) return ENOMEM; 
        }
        return EINVAL;
    }
    
    if (count < 2) {
        return 0; 
    }

    char *local_error_message = NULL;
    int local_error_code = 0;

    fmanage *fm = (fmanage *) salloc(sizeof(fmanage), NULL);
    if (!fm) { 
        local_error_message = sstrdup("Failed to allocate fmanage structure.", NULL); 
        local_error_code = ENOMEM; 
        if (error_message_out) *error_message_out = local_error_message;
        else if (local_error_message) free(local_error_message);
        return local_error_code;
    }
    fm_init(fm, max_open_files > 0 ? max_open_files : count);

    cmpdata current_cmp_data; 
    int cmp_init_ret = cmp_init(&current_cmp_data, count, max_buffer_per_file);

    if (cmp_init_ret != 0) {
        free(fm);
        local_error_code = cmp_init_ret;
        if (local_error_code == ENOMEM) {
             local_error_message = sstrdup("Failed to initialize comparison data structures (ENOMEM).", NULL);
        } else if (local_error_code == EINVAL) {
             local_error_message = sstrdup("Invalid arguments for comparison data initialization (e.g., buffer size too small).", NULL);
        } else {
             local_error_message = sstrdup("Unknown error during comparison data initialization.", NULL);
        }
        cmp_free(&current_cmp_data); 
        if (error_message_out) *error_message_out = local_error_message;
        else if (local_error_message) free(local_error_message);
        return local_error_code;
    }
    
    int overall_data_read_in_pass;
    do {
        overall_data_read_in_pass = 0;
        int current_file_idx_overall = 0;

        while(current_file_idx_overall < count) {
            int group_start_idx_in_order_array = current_file_idx_overall;
            size_t group_size = 0;
            while (current_file_idx_overall < count &&
                   cmp_uf_ordered_same(&current_cmp_data, group_start_idx_in_order_array, current_file_idx_overall)) {
                current_file_idx_overall++;
                group_size++;
            }

            if (group_size > 1) {
                size_t min_positive_read_in_batch = (size_t)-1; 
                int any_positive_data_read = 0;

                for (int i = 0; i < group_size; i++) {
                    int original_file_index = current_cmp_data.order[group_start_idx_in_order_array + i];
                    
                    if (current_cmp_data.file[original_file_index] == NULL) {
                        current_cmp_data.file[original_file_index] = fm_fopen(fm, file_paths[original_file_index]);
                        if (current_cmp_data.file[original_file_index] == NULL) {
                            if (local_error_code == 0) { 
                                local_error_code = errno;
                                char err_buf[256];
                                if (snprintf(err_buf, sizeof(err_buf), "Cannot open file '%s': %s", file_paths[original_file_index], strerror(errno)) > 0) {
                                    if (local_error_message) free(local_error_message);
                                    local_error_message = sstrdup(err_buf, NULL);
                                } else {
                                    if (local_error_message) free(local_error_message);
                                    local_error_message = sstrdup("Cannot open file (error message formatting failed).", NULL);
                                }
                                if (!local_error_message && local_error_code != 0 && local_error_code != ENOMEM) { /* sstrdup failed, but local_error_code is already errno */ }
                                else if (!local_error_message) { local_error_code = ENOMEM; }
                            }
                            continue; 
                        }
                    }

                    if (current_cmp_data.file[original_file_index]->_errno == 0) {
                         size_t bytes_read_this_file = fm_fread(fm, current_cmp_data.data[original_file_index], 1,
                                                                current_cmp_data.buffer_size, current_cmp_data.file[original_file_index]);

                        if (bytes_read_this_file > 0) {
                            any_positive_data_read = 1;
                            if (bytes_read_this_file < min_positive_read_in_batch) {
                                min_positive_read_in_batch = bytes_read_this_file;
                            }
                        } else { 
                            if (current_cmp_data.file[original_file_index]->_errno != 0) {
                                if (local_error_code == 0) {
                                    local_error_code = current_cmp_data.file[original_file_index]->_errno;
                                     char err_buf[256];
                                     if (snprintf(err_buf, sizeof(err_buf), "Error reading file '%s': %s", file_paths[original_file_index], strerror(local_error_code)) > 0) {
                                        if (local_error_message) free(local_error_message);
                                        local_error_message = sstrdup(err_buf, NULL);
                                    } else {
                                        if (local_error_message) free(local_error_message);
                                        local_error_message = sstrdup("Error reading file (error message formatting failed).", NULL);
                                    }
                                     if (!local_error_message && local_error_code != 0 && local_error_code != ENOMEM) { /* sstrdup failed */ }
                                     else if (!local_error_message) { local_error_code = ENOMEM; }
                                }
                            }
                        }
                    } else { 
                        if (local_error_code == 0 && current_cmp_data.file[original_file_index] != NULL ) { 
                           // Capture pre-existing error if no other error has been captured yet.
                           local_error_code = current_cmp_data.file[original_file_index]->_errno;
                           char err_buf[256];
                           if (snprintf(err_buf, sizeof(err_buf), "Pre-existing error for file '%s': %s", file_paths[original_file_index], strerror(local_error_code)) > 0) {
                               if (local_error_message) free(local_error_message);
                               local_error_message = sstrdup(err_buf, NULL);
                           } else {
                               if (local_error_message) free(local_error_message);
                               local_error_message = sstrdup("Pre-existing error (msg formatting failed).", NULL);
                           }
                           if (!local_error_message && local_error_code != 0 && local_error_code != ENOMEM) { /* sstrdup failed */ }
                           else if (!local_error_message) { local_error_code = ENOMEM; }
                        }
                    }
                } 

                size_t effective_read_for_batch = 0;
                if (any_positive_data_read) {
                    effective_read_for_batch = min_positive_read_in_batch;
                    overall_data_read_in_pass += effective_read_for_batch; 
                }
                
                current_cmp_data.readed = effective_read_for_batch;

                if (group_size > 1) { 
                    #if defined(_WIN32) || defined(_WIN64)
                        // Windows: use qsort_s. The context argument is last, similar to GNU qsort_r.
                        // errno_t qsort_s(void *base, rsize_t nmemb, rsize_t size, int (*compar)(const void *k1, const void *k2, void *context), void *context);
                        // We are not checking the errno_t return value for now, to keep it similar to the qsort_r void return.
                        qsort_s(&current_cmp_data.order[group_start_idx_in_order_array], group_size, sizeof(int), ufsorter, &current_cmp_data);
                    #elif defined(__APPLE__)
                        // macOS (BSD variant of qsort_r): context is the 4th argument, compar is the 5th.
                        qsort_r(&current_cmp_data.order[group_start_idx_in_order_array], group_size, sizeof(int), &current_cmp_data, ufsorter);
                    #else
                        // Linux/other (GNU qsort_r): compar is the 4th argument, context is the 5th.
                        qsort_r(&current_cmp_data.order[group_start_idx_in_order_array], group_size, sizeof(int), ufsorter, &current_cmp_data);
                    #endif
                }
            } 
        } 
    } while (overall_data_read_in_pass > 0);

    if (local_error_code == 0) { 
        int current_idx_in_order = 0;
        while (current_idx_in_order < count) {
            int group_start_sidx = current_idx_in_order; 
            current_idx_in_order++;
            while (current_idx_in_order < count && 
                   cmp_uf_ordered_same(&current_cmp_data, group_start_sidx, current_idx_in_order)) {
                current_idx_in_order++;
            }
            int num_in_potential_group = current_idx_in_order - group_start_sidx;

            if (num_in_potential_group > 1) {
                DuplicateSet current_set;
                current_set.paths = (char **)salloc(num_in_potential_group * sizeof(char *), NULL); // Pass NULL
                
                if (!current_set.paths) {
                    if (!local_error_message) local_error_message = sstrdup("Failed to allocate paths for a duplicate set callback.", NULL);
                    if (!local_error_code) local_error_code = ENOMEM;
                    break; 
                }

                int actual_paths_added = 0;
                for (int k = 0; k < num_in_potential_group; ++k) {
                    int original_file_idx = current_cmp_data.order[group_start_sidx + k];
                    
                    if (current_cmp_data.file[original_file_idx] != NULL && 
                        current_cmp_data.file[original_file_idx]->_errno == 0) {
                        
                        current_set.paths[actual_paths_added] = sstrdup(file_paths[original_file_idx], NULL); // Pass NULL
                        if (!current_set.paths[actual_paths_added]) {
                             if (!local_error_message) local_error_message = sstrdup("Failed to sstrdup file path for callback.", NULL);
                             if (!local_error_code) local_error_code = ENOMEM;
                             for(int j=0; j < actual_paths_added; ++j) free(current_set.paths[j]);
                             free(current_set.paths);
                             current_set.paths = NULL; 
                             goto cleanup_after_callback_error; 
                        }
                        actual_paths_added++;
                    }
                }
                
                if (actual_paths_added > 1) { 
                    current_set.count = actual_paths_added;
                    if (actual_paths_added < num_in_potential_group && actual_paths_added > 0) {
                        char **shrunk_paths = (char **)realloc(current_set.paths, actual_paths_added * sizeof(char *));
                        if (shrunk_paths) {
                            current_set.paths = shrunk_paths;
                        } 
                    }
                    callback(&current_set, user_data);
                }

                if (current_set.paths) {
                    for (int k = 0; k < actual_paths_added; k++) {
                        if (current_set.paths[k]) free(current_set.paths[k]);
                    }
                    free(current_set.paths);
                }
            }
        }
    }

cleanup_after_callback_error:;
    for (int i = 0; i < count; i++) {
        if (current_cmp_data.file[i] != NULL) {
            fm_fclose(fm, current_cmp_data.file[i]);
            current_cmp_data.file[i] = NULL;
        }
    }
    cmp_free(&current_cmp_data);
    fm_free(fm); 
    free(fm);    

    if (error_message_out && local_error_message) {
        *error_message_out = local_error_message; 
    } else if (local_error_message) {
        free(local_error_message); 
    }
    
    return local_error_code;
}