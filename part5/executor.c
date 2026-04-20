/* 
 *    Programmed By: Mohammed Isam [mohammed_isam1984@yahoo.com]
 *    Copyright 2020 (c)
 *    Refactored for safety and maintainability
 * 
 *    file: executor.c
 *    This file is part of the "Let's Build a Linux Shell" tutorial.
 *
 *    This tutorial is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This tutorial is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this tutorial.  If not, see <http://www.gnu.org/licenses/>.
 */    

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include "shell.h"
#include "node.h"
#include "executor.h"
#include "constants.h"
#include "error.h"
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#define PATHSEP ';'
#else
#define PATHSEP ':'
#endif


/**
 * Search for a command in the PATH environment variable
 * Returns allocated path string if found, NULL otherwise
 */
char *search_path(const char *file)
{
    if (!file || !*file) {
        errno = ENOENT;
        return NULL;
    }
    
    const char *PATH = getenv("PATH");
    if (!PATH || !*PATH) {
        errno = ENOENT;
        return NULL;
    }
    
    /* Create a working copy of PATH since strtok modifies it */
    char *path_copy = shell_strdup_bounded(PATH, MAX_PATH_COMPONENT * 10);
    if (!path_copy) {
        errno = ENOMEM;
        return NULL;
    }
    
    char *result = NULL;
    char full_path[MAX_PATH_COMPONENT + 1];
    
    char *saveptr;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    
    while (dir) {
        /* Skip empty path components */
        if (!*dir) {
            dir = strtok_r(NULL, ":", &saveptr);
            continue;
        }
        
        /* Build the full path - use snprintf for safety */
        int chars_written = snprintf(full_path, sizeof(full_path), 
                                     "%s/%s", dir, file);
        
        /* Check for buffer overflow */
        if (chars_written < 0 || chars_written >= (int)sizeof(full_path)) {
            SHELL_WARN("path component too long: %s", dir);
            dir = strtok_r(NULL, ":", &saveptr);
            continue;
        }
        
        /* Check if file exists and is executable */
        if (is_executable(full_path)) {
            result = shell_strdup_bounded(full_path, MAX_PATH_COMPONENT);
            break;
        }
        
        dir = strtok_r(NULL, ":", &saveptr);
    }
    
    free(path_copy);
    
    if (!result) {
        errno = ENOENT;
    }
    
    return result;
}


/**
 * Execute a command by finding it in PATH and calling execv
 */
static int levenshtein_distance_limited(const char *a, const char *b, int max_distance)
{
    int len_a = strlen(a);
    int len_b = strlen(b);
    if (abs(len_a - len_b) > max_distance) {
        return max_distance + 1;
    }

    int *prev = shell_malloc((len_b + 1) * sizeof(int));
    int *curr = shell_malloc((len_b + 1) * sizeof(int));
    if (!prev || !curr) {
        free(prev);
        free(curr);
        return max_distance + 1;
    }

    for (int j = 0; j <= len_b; j++) {
        prev[j] = j;
    }

    for (int i = 1; i <= len_a; i++) {
        curr[0] = i;
        int min_row = curr[0];

        for (int j = 1; j <= len_b; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int deletion = prev[j] + 1;
            int insertion = curr[j - 1] + 1;
            int substitution = prev[j - 1] + cost;
            int distance = deletion;

            if (insertion < distance) {
                distance = insertion;
            }
            if (substitution < distance) {
                distance = substitution;
            }

            curr[j] = distance;
            if (distance < min_row) {
                min_row = distance;
            }
        }

        if (min_row > max_distance) {
            int *tmp = prev;
            prev = curr;
            curr = tmp;
            continue;
        }

        int *tmp = prev;
        prev = curr;
        curr = tmp;
    }

    int result = prev[len_b];
    free(prev);
    free(curr);
    return result;
}


static char *suggest_closest_command(const char *name)
{
    int best_distance = 3;
    char *best_match = NULL;

    for (int i = 0; i < builtins_count; i++) {
        int distance = levenshtein_distance_limited(name, builtins[i].name, best_distance);
        if (distance < best_distance) {
            free(best_match);
            best_match = shell_strdup_bounded(builtins[i].name, MAX_INPUT_BUFFER);
            if (!best_match) {
                return NULL;
            }
            best_distance = distance;
        }
    }

    const char *path_env = getenv("PATH");
    if (!path_env) {
        return best_match;
    }

    char *path_copy = shell_strdup_bounded(path_env, MAX_INPUT_BUFFER);
    if (!path_copy) {
        return best_match;
    }

    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);

    while (dir) {
        if (*dir == '\0') {
            dir = ".";
        }

        DIR *dp = opendir(dir);
        if (dp) {
            struct dirent *entry;
            while ((entry = readdir(dp)) != NULL) {
                if (entry->d_name[0] == '.') {
                    continue;
                }

                int candidate_len = strlen(entry->d_name);
                if (abs(candidate_len - (int)strlen(name)) > best_distance) {
                    continue;
                }

                char fullpath[MAX_PATH_COMPONENT + 1];
                int len = snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
                if (len < 0 || len >= (int)sizeof(fullpath)) {
                    continue;
                }

                if (!is_executable(fullpath)) {
                    continue;
                }

                int distance = levenshtein_distance_limited(name, entry->d_name, best_distance);
                if (distance < best_distance) {
                    free(best_match);
                    best_match = shell_strdup_bounded(entry->d_name, MAX_INPUT_BUFFER);
                    if (!best_match) {
                        closedir(dp);
                        free(path_copy);
                        return NULL;
                    }
                    best_distance = distance;
                }
            }
            closedir(dp);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return best_match;
}


int do_exec_cmd(int argc, char **argv)
{
    if (!argv || !argv[0]) {
        SHELL_ERROR("do_exec_cmd: invalid arguments");
        return 0;
    }
    
    char *path_to_exec = argv[0];
    
    /* If argv[0] contains a path separator, use it directly */
    if (has_path_separator(argv[0])) {
        execv(argv[0], argv);
    } else {
        /* Search for command in PATH */
        path_to_exec = search_path(argv[0]);
        if (!path_to_exec) {
            char *suggestion = suggest_closest_command(argv[0]);
            SHELL_ERROR("command not found: %s", argv[0]);
            if (suggestion) {
                SHELL_WARN("did you mean: %s", suggestion);
                free(suggestion);
            }
            return 0;
        }
        
        execv(path_to_exec, argv);
        free(path_to_exec);
    }
    
    return 0;
}


int do_simple_command(struct node_s *node)
{
    if(!node)
    {
        return 0;
    }

    struct node_s *child = node->first_child;
    if(!child)
    {
        return 0;
    }
    
    int argc = 0;           /* arguments count */
    int targc = 0;          /* total alloc'd arguments count */
    char **argv = NULL;
    char *str;

    while(child)
    {
        str = child->val.str;
        /*perform word expansion */
        struct word_s *w = word_expand(str);
        
        /* word expansion failed */
        if(!w)
        {
            child = child->next_sibling;
            continue;
        }

        /* add the words to the arguments list */
        struct word_s *w2 = w;
        while(w2)
        {
            if(check_buffer_bounds(&argc, &targc, &argv))
            {
                str = malloc(strlen(w2->data)+1);
                if(str)
                {
                    strcpy(str, w2->data);
                    argv[argc++] = str;
                }
            }
            w2 = w2->next;
        }
        
        /* free the memory used by the expanded words */
        free_all_words(w);
        
        /* check the next word */
        child = child->next_sibling;
    }

    /* even if arc == 0, we need to alloc memory for argv */
    if(check_buffer_bounds(&argc, &targc, &argv))
    {
        /* NULL-terminate the array */
        argv[argc] = NULL;
    }

    int i = 0;
    for( ; i < builtins_count; i++)
    {
        if(strcmp(argv[0], builtins[i].name) == 0)
        {
            builtins[i].func(argc, argv);
            free_argv(argc, argv);
            return 1;
        }
    }

#ifdef _WIN32
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    char cmdline[MAX_CMDLINE_BUFFER] = "";
    
    /* Build command line safely */
    for(int i = 0; i < argc; i++) {
        if(i > 0) {
            if (shell_strlcat(cmdline, " ", sizeof(cmdline)) != 0) {
                SHELL_ERROR("command line too long for Windows execution");
                free_argv(argc, argv);
                return 0;
            }
        }
        if (shell_strlcat(cmdline, argv[i], sizeof(cmdline)) != 0) {
            SHELL_ERROR("command line too long for Windows execution");
            free_argv(argc, argv);
            return 0;
        }
    }
    
    if(!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        SHELL_ERROR("failed to create process");
        free_argv(argc, argv);
        return 0;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    free_argv(argc, argv);
    return 1;
#else
    pid_t child_pid = 0;
    if((child_pid = fork()) == 0)
    {
        do_exec_cmd(argc, argv);
        SHELL_PERROR("failed to execute command");
        if(errno == ENOEXEC)
        {
            exit(SHELL_EX_NOEXEC);
        }
        else if(errno == ENOENT)
        {
            exit(SHELL_EX_NOTFOUND);
        }
        else
        {
            exit(EXIT_FAILURE);
        }
    }
    else if(child_pid < 0)
    {
        SHELL_PERROR("failed to fork command");
        free_argv(argc, argv);
        return 0;
    }

    int status = 0;
    waitpid(child_pid, &status, 0);
    free_argv(argc, argv);
    
    return 1;
#endif
}
