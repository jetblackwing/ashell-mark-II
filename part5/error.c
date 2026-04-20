/* 
 *    A-Shell Project - Refactored
 *    file: error.c
 *    
 *    Implementation of centralized error handling functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "error.h"
#include "constants.h"

/**
 * Handle a critical error and exit the shell
 */
void shell_fatal_error(int exit_code, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    fprintf(stderr, "ashell: fatal error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    
    exit(exit_code);
}

/**
 * Check if system call returned error
 */
int check_sys_error(int result, const char *syscall_name)
{
    if (result < 0) {
        SHELL_PERROR("system call '%s' failed", syscall_name);
        return 1;
    }
    return 0;
}

/**
 * Handle memory allocation failure
 */
void* handle_malloc_error(void *ptr, size_t size)
{
    if (!ptr) {
        fprintf(stderr, "ashell: error: failed to allocate %zu bytes: %s\n", 
                size, strerror(errno));
        errno = ENOMEM;
    }
    return ptr;
}
