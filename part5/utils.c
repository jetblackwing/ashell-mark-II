/* 
 *    A-Shell Project - Refactored
 *    file: utils.c
 *    
 *    Implementation of common utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"
#include "error.h"
#include "constants.h"

/* ==========================
   Memory Management Utilities
   ========================== */

void* shell_malloc(size_t size)
{
    if (size == 0) {
        SHELL_ERROR("attempting to allocate 0 bytes");
        return NULL;
    }
    
    void *ptr = malloc(size);
    if (!ptr) {
        SHELL_ERROR("memory allocation failed (%zu bytes)", size);
        errno = ENOMEM;
    }
    return ptr;
}

void* shell_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        SHELL_ERROR("attempting to reallocate to 0 bytes");
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        SHELL_ERROR("memory reallocation failed (%zu bytes)", size);
        errno = ENOMEM;
    }
    return new_ptr;
}

void free_argv(int argc, char **argv)
{
    if (!argc || !argv) {
        return;
    }
    
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            free(argv[i]);
        }
    }
    free(argv);
}

/* ==========================
   String Utilities
   ========================== */

char* shell_strdup_bounded(const char *str, size_t max_len)
{
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str);
    if (max_len > 0 && len > max_len) {
        SHELL_ERROR("string too long (max %zu bytes)", max_len);
        return NULL;
    }
    
    char *dup = malloc(len + 1);
    if (!dup) {
        SHELL_ERROR("memory allocation failed for string duplication");
        return NULL;
    }
    
    strcpy(dup, str);
    return dup;
}

int shell_strlcat(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    /* Check if concatenation would overflow */
    if (dest_len + src_len >= dest_size) {
        SHELL_ERROR("string concatenation would overflow (need %zu, have %zu)", 
                   dest_len + src_len + 1, dest_size);
        return -1;
    }
    
    strcat(dest, src);
    return 0;
}

int shell_strlcpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t src_len = strlen(src);
    
    /* Check if copy would overflow */
    if (src_len >= dest_size) {
        SHELL_ERROR("string copy would overflow (need %zu, have %zu)", 
                   src_len + 1, dest_size);
        return -1;
    }
    
    strcpy(dest, src);
    return 0;
}

/* ==========================
   Path Utilities
   ========================== */

int has_path_separator(const char *path)
{
    if (!path) {
        return 0;
    }
    return strchr(path, '/') != NULL;
}

int is_executable(const char *path)
{
    if (!path) {
        return 0;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    /* Check if it's a regular file and readable */
    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}
