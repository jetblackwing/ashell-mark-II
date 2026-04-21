/* 
 *    A-Shell Project - Additional Builtins
 *    Copyright 2024 (c)
 * 
 *    file: builtins/shell_builtins.c
 *    Additional shell builtin commands: cd, pwd, export, unset
 *
 *    This file is part of the A-Shell project.
 *    Licensed under GNU General Public License v3.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include "../shell.h"
#include "../error.h"
#include "../utils.h"
#include "../symtab/symtab.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/**
 * Built-in: cd - Change directory
 * Usage: cd [directory]
 * If no argument provided, changes to home directory
 */
int shell_cd(int argc, char **argv)
{
    const char *target_dir = NULL;
    
    /* Get target directory */
    if (argc < 2) {
        /* No argument: cd to home */
        target_dir = getenv("HOME");
        if (!target_dir) {
            SHELL_ERROR("cd: HOME not set");
            return 1;
        }
    } else {
        target_dir = argv[1];
    }
    
    /* Change directory */
    if (chdir(target_dir) != 0) {
        SHELL_PERROR("cd: cannot change to directory '%s'", target_dir);
        return 1;
    }
    
    /* Update PWD in symbol table */
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }
    
    return 0;
}

/**
 * Built-in: pwd - Print working directory
 * Usage: pwd
 */
int shell_pwd(int argc, char **argv)
{
    (void)argc;  /* Unused */
    (void)argv;  /* Unused */
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        SHELL_PERROR("pwd: cannot get current directory");
        return 1;
    }
    
    printf("%s\n", cwd);
    return 0;
}

/**
 * Built-in: export - Export variable to environment
 * Usage: export VAR=value
 *        export VAR (exports existing variable)
 */
int shell_export(int argc, char **argv)
{
    if (argc < 2) {
        SHELL_ERROR("export: missing operand");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        const char *var = argv[i];
        const char *eq = strchr(var, '=');
        
        if (eq) {
            /* export VAR=value */
            size_t name_len = eq - var;
            char name[256];
            
            if (name_len >= sizeof(name)) {
                SHELL_ERROR("export: variable name too long");
                continue;
            }
            
            strncpy(name, var, name_len);
            name[name_len] = '\0';
            
            if (setenv(name, eq + 1, 1) != 0) {
                SHELL_PERROR("export: setenv failed");
            }
        } else {
            /* export VAR (export existing) */
            const char *value = getenv(var);
            if (value) {
                setenv(var, value, 1);
            } else {
                SHELL_WARN("export: %s: variable not found", var);
            }
        }
    }
    
    return 0;
}

/**
 * Built-in: unset - Unset a variable
 * Usage: unset VAR [VAR ...]
 */
int shell_unset(int argc, char **argv)
{
    if (argc < 2) {
        SHELL_ERROR("unset: missing operand");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (unsetenv(argv[i]) != 0) {
            SHELL_PERROR("unset: unsetenv failed for '%s'", argv[i]);
        }
    }
    
    return 0;
}

/**
 * Built-in: echo - Print arguments
 * Usage: echo [args...]
 * Note: Simple implementation (no escape sequence processing)
 */
int shell_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    
    return 0;
}

/**
 * Built-in: type - Show command type
 * Usage: type cmd [cmd ...]
 */
int shell_type(int argc, char **argv)
{
    if (argc < 2) {
        SHELL_ERROR("type: missing operand");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        const char *cmd = argv[i];
        
        /* Check if it's a builtin */
        int is_builtin = 0;
        for (int j = 0; j < builtins_count; j++) {
            if (strcmp(cmd, builtins[j].name) == 0) {
                printf("%s is a shell builtin\n", cmd);
                is_builtin = 1;
                break;
            }
        }
        
        if (!is_builtin) {
            /* Check if it's an external command */
            extern char *search_path(const char *file);
            char *path = search_path(cmd);
            if (path) {
                printf("%s is %s\n", cmd, path);
                free(path);
            } else {
                printf("%s: not found\n", cmd);
            }
        }
    }
    
    return 0;
}

/*
 * Built-in: saveconfig - write minimal shell configuration (PS1, PS2)
 * Usage: saveconfig
 */
int shell_saveconfig(int argc, char **argv)
{
    (void)argc; (void)argv;

    const char *confdir = "../config";
    const char *confpath = "../config/shell.conf";

    struct stat st;
    if (stat(confdir, &st) != 0) {
        if (mkdir(confdir, 0755) != 0) {
            SHELL_PERROR("saveconfig: cannot create config directory '%s'", confdir);
            return 1;
        }
    }

    FILE *f = fopen(confpath, "w");
    if (!f) {
        SHELL_PERROR("saveconfig: cannot open config file '%s' for writing", confpath);
        return 1;
    }

    struct symtab_entry_s *entry;

    entry = get_symtab_entry("PS1");
    if (entry && entry->val) {
        fprintf(f, "PS1=%s\n", entry->val);
    }

    entry = get_symtab_entry("PS2");
    if (entry && entry->val) {
        fprintf(f, "PS2=%s\n", entry->val);
    }

    fclose(f);
    return 0;
}
