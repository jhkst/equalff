#ifndef _FCOMPARE_H
#define _FCOMPARE_H

#include <stdio.h>
#include <stdlib.h>

// salloc.h is needed for sstrdup, and its handle_exit_func type if we want to make it configurable
#include "salloc.h"

// Represents a single set of duplicate files
typedef struct {
    char **paths;       // Array of file path strings (must be freed by caller via free_comparison_result)
    int count;          // Number of paths in this set
} DuplicateSet;

// Represents the overall result of a comparison operation
typedef struct {
    DuplicateSet *sets; // Array of DuplicateSet structures
    int count;          // Number of duplicate sets found
    int error_code;     // 0 on success, non-zero on error
    char *error_message; // Description of the error (must be freed if not NULL)
} ComparisonResult;

/**
 * Compares a list of files (assumed to be of the same size and count > 1)
 * to find sets of identical files among them.
 *
 * @param name Array of file paths.
 * @param count Number of files in the name array.
 * @param max_buffer Maximum memory buffer to use for comparison per file.
 * @param max_open_files Maximum number of files to keep open simultaneously.
 * @return A pointer to a ComparisonResult structure. The caller is responsible
 *         for freeing this structure using free_comparison_result().
 *         Returns NULL if a catastrophic error occurs (e.g., initial memory allocation failure).
 */
ComparisonResult* compare_files(char *name[], int count, int max_buffer, int max_open_files);

/**
 * Frees the memory allocated for a ComparisonResult structure and its contents.
 *
 * @param result The ComparisonResult structure to free.
 */
void free_comparison_result(ComparisonResult *result);

/**
 * Frees the memory allocated for an error message string by API functions.
 *
 * @param error_message The error message string to free.
 */
void free_error_message(char *error_message);

// Callback function type: invoked when a set of duplicate files is found.
// The 'duplicates' structure and its 'paths' are valid only for the duration of the callback.
// If the user needs to retain this information, they must copy it.
// The 'paths' array within 'duplicates' will be NULL-terminated for convenience if a count is not enough.
// 'user_data' is the arbitrary pointer passed to compare_files_async.
typedef void (*DuplicateFoundCallback)(const DuplicateSet *duplicates, void *user_data);

/**
 * Compares a list of files (assumed to be of the same size and count > 1)
 * to find sets of identical files among them.
 * This version invokes a callback for each set of duplicates found.
 *
 * @param file_paths Array of file paths.
 * @param count Number of files in the file_paths array.
 * @param max_buffer_per_file Maximum memory buffer to use for comparison per file.
 * @param max_open_files Maximum number of files to keep open simultaneously.
 * @param callback The callback function to be invoked for each duplicate set.
 * @param user_data Arbitrary data pointer to be passed to the callback.
 * @param error_message_out Pointer to a char* that will be allocated and filled with
 *                          an error message if the function returns a non-zero value.
 *                          The caller is responsible for freeing this message using
 *                          free_error_message() if it's not NULL.
 * @return 0 on success, non-zero on error. If non-zero, error_message_out may contain
 *         a description of the error.
 */
int compare_files_async(
    char *file_paths[],
    int count,
    int max_buffer_per_file,
    int max_open_files,
    DuplicateFoundCallback callback,
    void *user_data,
    char **error_message_out
);

#endif