/* 
 *    A-Shell Project - Refactored
 *    file: error.h
 *    
 *    Centralized error handling and reporting.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <errno.h>

/* ==========================
   Error Message Macros
   ========================== */

/* Print error with program context */
#define SHELL_ERROR(fmt, ...) \
    fprintf(stderr, "ashell: error: " fmt "\n", ##__VA_ARGS__)

/* Print error with system errno info */
#define SHELL_PERROR(fmt, ...) \
    fprintf(stderr, "ashell: error: " fmt ": %s\n", ##__VA_ARGS__, strerror(errno))

/* Print warning message */
#define SHELL_WARN(fmt, ...) \
    fprintf(stderr, "ashell: warning: " fmt "\n", ##__VA_ARGS__)

/* Print debug message (only if DEBUG_MODE enabled) */
#ifdef DEBUG_MODE
    #if DEBUG_MODE == 1
        #define SHELL_DEBUG(fmt, ...) \
            fprintf(stderr, "ashell: debug: " fmt "\n", ##__VA_ARGS__)
    #else
        #define SHELL_DEBUG(fmt, ...)
    #endif
#else
    #define SHELL_DEBUG(fmt, ...)
#endif

/* ==========================
   Error Handler Functions
   ========================== */

/**
 * Handle a critical error and exit the shell
 * @param exit_code - Exit code to use
 * @param fmt - Format string
 */
void shell_fatal_error(int exit_code, const char *fmt, ...);

/**
 * Check if system call returned error, print strerror and return
 * @param result - Return value from system call (-1 typically indicates error)
 * @param syscall_name - Name of the system call for error message
 * @return Non-zero if error occurred, 0 otherwise
 */
int check_sys_error(int result, const char *syscall_name);

/**
 * Handle memory allocation failure
 * @param ptr - Result pointer (NULL indicates failure)
 * @param size - Size attempted to allocate
 * @return The same pointer
 */
void* handle_malloc_error(void *ptr, size_t size);

#endif /* ERROR_H */
