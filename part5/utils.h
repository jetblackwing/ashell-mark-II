/* 
 *    A-Shell Project - Refactored
 *    file: utils.h
 *    
 *    Common utility functions and helpers.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* ==========================
   Memory Management Utilities
   ========================== */

/**
 * Safe memory allocation wrapper
 * @param size - Size to allocate
 * @return Allocated pointer or NULL on failure
 */
void* shell_malloc(size_t size);

/**
 * Safe memory reallocation wrapper
 * @param ptr - Existing pointer (can be NULL)
 * @param size - New size
 * @return Reallocated pointer or NULL on failure
 */
void* shell_realloc(void *ptr, size_t size);

/**
 * Free argument vector (argc-style)
 * @param argc - Number of arguments
 * @param argv - Argument vector
 */
void free_argv(int argc, char **argv);

/* ==========================
   String Utilities
   ========================== */

/**
 * Safe string duplication with bounds checking
 * @param str - String to duplicate
 * @param max_len - Maximum length (0 = no limit)
 * @return Duplicated string or NULL
 */
char* shell_strdup_bounded(const char *str, size_t max_len);

/**
 * Concatenate strings with bounds checking
 * @param dest - Destination buffer
 * @param src - Source string to append
 * @param dest_size - Size of destination buffer
 * @return 0 on success, -1 if would overflow
 */
int shell_strlcat(char *dest, const char *src, size_t dest_size);

/**
 * Copy string with bounds checking
 * @param dest - Destination buffer
 * @param src - Source string
 * @param dest_size - Size of destination buffer
 * @return 0 on success, -1 if would overflow
 */
int shell_strlcpy(char *dest, const char *src, size_t dest_size);

/* ==========================
   Path Utilities
   ========================== */

/**
 * Check if a path string contains directory separators
 * @param path - Path string to check
 * @return 1 if path contains '/', 0 otherwise
 */
int has_path_separator(const char *path);

/**
 * Check if a file is readable/executable
 * @param path - Full path to file
 * @return 1 if file is regular and readable, 0 otherwise
 */
int is_executable(const char *path);

#endif /* UTILS_H */
