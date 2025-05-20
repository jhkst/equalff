#ifndef _SALLOC_H
#define _SALLOC_H

#include <stdlib.h>
#include <string.h>

/**
 * Default error handler for salloc and sstrdup if a specific one is not provided
 * or if the provided one does not exit. Typically, this function prints an error
 * and exits the program.
 */
void handle_exit();

/**
 * Allocates memory of the given size.
 * If allocation fails and error_handle is not NULL, error_handle is called.
 * If allocation fails and error_handle is NULL, this function returns NULL.
 *
 * @param size The number of bytes to allocate.
 * @param error_handle A function pointer to an error handling routine.
 *                     If NULL, the function will return NULL on allocation failure.
 *                     If non-NULL, this handler is called on allocation failure.
 *                     The default `handle_exit` will terminate the program.
 * @return A pointer to the allocated memory, or NULL if allocation fails and error_handle is NULL.
 */
void *salloc(size_t size, void (*error_handle)());

/**
 * Duplicates a string using salloc for memory allocation.
 * If allocation fails and error_handle is not NULL, error_handle is called.
 * If allocation fails and error_handle is NULL, this function returns NULL.
 *
 * @param s The string to duplicate.
 * @param error_handle A function pointer to an error handling routine.
 *                     If NULL, the function will return NULL on allocation failure.
 *                     If non-NULL, this handler is called on allocation failure.
 *                     The default `handle_exit` will terminate the program.
 * @return A pointer to the newly allocated duplicated string, or NULL if allocation fails and error_handle is NULL.
 */
char *sstrdup(const char *s, void (*error_handle)());

#endif
