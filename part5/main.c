/* 
 *    Programmed By: jetblackwing [amaljk80@gmail.com]
 *    Copyright 2026 (c) jetblackwing [amaljk80@gmail.com]
 *    Refactored for safety and maintainability
 * 
 *    file: main.c
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
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <termios.h>
#endif
#include <unistd.h>
#include "shell.h"
#include "source.h"
#include "parser.h"
#include "executor.h"
#include "constants.h"
#include "error.h"
#include "utils.h"

/* Global history buffer */
static char *history[MAX_HISTORY_SIZE];
static int history_count = 0;

/**
 * Add a command to the history buffer
 */
static void add_to_history(const char *cmd) 
{
    if (!cmd || !*cmd) {
        return;
    }
    
    char *line = shell_strdup_bounded(cmd, MAX_INPUT_BUFFER);
    if (!line) {
        return;
    }
    
    /* Remove trailing newline */
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    if (history_count < MAX_HISTORY_SIZE) {
        history[history_count++] = line;
    } else {
        /* Circular buffer: free oldest and shift */
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY_SIZE - 1) * sizeof(char*));
        history[MAX_HISTORY_SIZE - 1] = line;
    }
}

/**
 * Cleanup history on exit
 */
static void cleanup_history(void)
{
    for (int i = 0; i < history_count; i++) {
        if (history[i]) {
            free(history[i]);
            history[i] = NULL;
        }
    }
    history_count = 0;
}


static int is_prefix(const char *string, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    return (prefix_len == 0 || strncmp(string, prefix, prefix_len) == 0);
}


static void clear_current_line(void)
{
    fprintf(stderr, "\r\033[K");
}


static void redraw_current_line(const char *buffer)
{
    clear_current_line();
    print_prompt1();
    fprintf(stderr, "%s", buffer);
    fprintf(stderr, "\033[K");
}


static int add_completion_candidate(char ***candidates, size_t *candidate_count, const char *candidate)
{
    if (!candidate || !*candidate) {
        return 0;
    }

    for (size_t i = 0; i < *candidate_count; i++) {
        if (strcmp((*candidates)[i], candidate) == 0) {
            return 1;
        }
    }

    char **tmp = shell_realloc(*candidates, (*candidate_count + 1) * sizeof(char *));
    if (!tmp) {
        return 0;
    }

    char *entry = shell_strdup_bounded(candidate, MAX_INPUT_BUFFER);
    if (!entry) {
        return 0;
    }

    *candidates = tmp;
    (*candidates)[*candidate_count] = entry;
    (*candidate_count)++;
    return 1;
}


static void free_completion_candidates(char **candidates, size_t candidate_count)
{
    for (size_t i = 0; i < candidate_count; i++) {
        free(candidates[i]);
    }
    free(candidates);
}


static void display_completion_candidates(char **candidates, size_t candidate_count)
{
    fprintf(stderr, "\n");
    for (size_t i = 0; i < candidate_count; i++) {
        fprintf(stderr, "%s\n", candidates[i]);
    }
}


static void get_common_prefix(char *output, char **candidates, size_t candidate_count)
{
    if (candidate_count == 0) {
        output[0] = '\0';
        return;
    }

    shell_strlcpy(output, candidates[0], MAX_INPUT_BUFFER);

    for (size_t i = 1; i < candidate_count; i++) {
        size_t j = 0;
        while (output[j] && candidates[i][j] && output[j] == candidates[i][j]) {
            j++;
        }
        output[j] = '\0';
    }
}


static int collect_builtin_matches(const char *prefix, char ***candidates, size_t *candidate_count)
{
    for (int i = 0; i < builtins_count; i++) {
        if (is_prefix(builtins[i].name, prefix)) {
            add_completion_candidate(candidates, candidate_count, builtins[i].name);
        }
    }
    return 1;
}


static int collect_history_matches(const char *prefix, char ***candidates, size_t *candidate_count)
{
    for (int i = 0; i < history_count; i++) {
        if (is_prefix(history[i], prefix)) {
            add_completion_candidate(candidates, candidate_count, history[i]);
        }
    }
    return 1;
}


static int collect_path_matches(const char *prefix, char ***candidates, size_t *candidate_count)
{
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return 0;
    }

    char *path_copy = shell_strdup_bounded(path_env, MAX_INPUT_BUFFER);
    if (!path_copy) {
        return 0;
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
                if (entry->d_name[0] == '.' || !is_prefix(entry->d_name, prefix)) {
                    continue;
                }

                char fullpath[MAX_PATH_COMPONENT + 1];
                int len = snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
                if (len < 0 || len >= (int)sizeof(fullpath)) {
                    continue;
                }

                if (is_executable(fullpath)) {
                    add_completion_candidate(candidates, candidate_count, entry->d_name);
                }
            }
            closedir(dp);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return 1;
}


static int collect_local_executable_matches(const char *prefix, char ***candidates, size_t *candidate_count)
{
    DIR *dp = opendir(".");
    if (!dp) {
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.' || !is_prefix(entry->d_name, prefix)) {
            continue;
        }

        char fullpath[MAX_PATH_COMPONENT + 1];
        int len = snprintf(fullpath, sizeof(fullpath), "./%s", entry->d_name);
        if (len < 0 || len >= (int)sizeof(fullpath)) {
            continue;
        }

        if (is_executable(fullpath)) {
            add_completion_candidate(candidates, candidate_count, entry->d_name);
        }
    }

    closedir(dp);
    return 1;
}


static int collect_filesystem_matches(const char *prefix, char ***candidates, size_t *candidate_count)
{
    const char *slash = strrchr(prefix, '/');
    char dirpath[MAX_INPUT_BUFFER];
    char basename[MAX_INPUT_BUFFER];

    if (slash) {
        size_t dir_len = slash - prefix;
        if (dir_len == 0) {
            shell_strlcpy(dirpath, "/", sizeof(dirpath));
        } else {
            if (dir_len >= sizeof(dirpath)) {
                return 0;
            }
            memcpy(dirpath, prefix, dir_len);
            dirpath[dir_len] = '\0';
        }
        shell_strlcpy(basename, slash + 1, sizeof(basename));
    } else {
        shell_strlcpy(dirpath, ".", sizeof(dirpath));
        shell_strlcpy(basename, prefix, sizeof(basename));
    }

    DIR *dp = opendir(dirpath);
    if (!dp) {
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.' || !is_prefix(entry->d_name, basename)) {
            continue;
        }

        char candidate[MAX_INPUT_BUFFER];
        int len;
        if (slash) {
            len = snprintf(candidate, sizeof(candidate), "%s/%s", dirpath, entry->d_name);
        } else {
            len = snprintf(candidate, sizeof(candidate), "%s", entry->d_name);
        }
        if (len < 0 || len >= (int)sizeof(candidate)) {
            continue;
        }

        add_completion_candidate(candidates, candidate_count, candidate);
    }

    closedir(dp);
    return 1;
}


static int get_token_start(const char *buffer, int bufpos)
{
    int pos = bufpos - 1;

    while (pos >= 0 && !isspace((unsigned char)buffer[pos])) {
        pos--;
    }

    return pos + 1;
}


static int complete_current_token(char *buffer, int *bufpos, char *suggestion, size_t suggestion_size)
{
    int token_start = get_token_start(buffer, *bufpos);
    char prefix[MAX_INPUT_BUFFER];
    size_t prefix_len = *bufpos - token_start;
    if (prefix_len >= sizeof(prefix)) {
        return 0;
    }

    memcpy(prefix, buffer + token_start, prefix_len);
    prefix[prefix_len] = '\0';

    if (prefix_len == 0) {
        return 0;
    }

    char **matches = NULL;
    size_t match_count = 0;

    if (strchr(prefix, '/')) {
        collect_filesystem_matches(prefix, &matches, &match_count);
    } else {
        collect_builtin_matches(prefix, &matches, &match_count);
        collect_history_matches(prefix, &matches, &match_count);
        collect_path_matches(prefix, &matches, &match_count);
        collect_local_executable_matches(prefix, &matches, &match_count);
    }

    if (match_count == 0) {
        free_completion_candidates(matches, match_count);
        return 0;
    }

    char common[MAX_INPUT_BUFFER];
    get_common_prefix(common, matches, match_count);
    size_t common_len = strlen(common);

    if (match_count == 1 || common_len > prefix_len) {
        size_t new_len = token_start + common_len;
        if (new_len >= MAX_INPUT_BUFFER - 1) {
            free_completion_candidates(matches, match_count);
            return 0;
        }

        memcpy(buffer + token_start, common, common_len);
        *bufpos = new_len;
        buffer[*bufpos] = '\0';

        if (match_count == 1) {
            shell_strlcpy(suggestion, matches[0] + prefix_len, suggestion_size);
        } else {
            shell_strlcpy(suggestion, common + prefix_len, suggestion_size);
        }

        free_completion_candidates(matches, match_count);
        return 1;
    }

    /* Show multiple completion candidates */
    display_completion_candidates(matches, match_count);
    free_completion_candidates(matches, match_count);
    return 0;
}


static char *find_history_suggestion(const char *prefix)
{
    for (int i = history_count - 1; i >= 0; i--) {
        if (is_prefix(history[i], prefix) && strlen(history[i]) > strlen(prefix)) {
            return history[i] + strlen(prefix);
        }
    }
    return NULL;
}


int main(int argc, char **argv)
{
    char *cmd;

    /* Register cleanup handler */
    atexit(cleanup_history);

    initsh();
    
    do
    {
        print_prompt1();
        cmd = read_cmd();
        if(!cmd)
        {
            exit(EXIT_SUCCESS);
        }
        if(cmd[0] == '\0' || strcmp(cmd, "\n") == 0)
        {
            free(cmd);
            continue;
        }
        if(strcmp(cmd, "exit\n") == 0)
        {
            free(cmd);
            break;
        }
        struct source_s src;
        src.buffer   = cmd;
        src.bufsize  = strlen(cmd);
        src.curpos   = INIT_SRC_POS;
        parse_and_execute(&src);
        add_to_history(cmd);
        free(cmd);
    } while(1);
    
    exit(EXIT_SUCCESS);
}


/**
 * Read a command from standard input
 * Uses platform-specific implementations for better terminal control
 */
char *read_cmd(void)
{
#ifdef _WIN32
    char buffer[MAX_INPUT_BUFFER];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return NULL;
    }
    return shell_strdup_bounded(buffer, MAX_INPUT_BUFFER);
#else
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    char buffer[MAX_INPUT_BUFFER] = {0};
    char current_line[MAX_INPUT_BUFFER] = {0};
    char suggestion[MAX_INPUT_BUFFER] = {0};
    int bufpos = 0;
    int history_index = history_count;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            return NULL;
        }

        /* Newline - end of command */
        if (c == '\n') {
            buffer[bufpos] = '\0';
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            fprintf(stderr, "\n");

            /* Create return string with newline */
            char *ret = shell_malloc(bufpos + 2);
            if (ret) {
                strcpy(ret, buffer);
                strcat(ret, "\n");
            }
            return ret;
        }

        /* Clear screen */
        else if (c == 12) {
            fprintf(stderr, "\033[2J\033[H");
            redraw_current_line(buffer);
            continue;
        }

        /* Tab completion */
        else if (c == '\t') {
            if (complete_current_token(buffer, &bufpos, suggestion, sizeof(suggestion))) {
                redraw_current_line(buffer);
                if (suggestion[0]) {
                    fprintf(stderr, "\033[90m%s\033[0m", suggestion);
                }
            } else if (suggestion[0]) {
                redraw_current_line(buffer);
                fprintf(stderr, "\033[90m%s\033[0m", suggestion);
            } else {
                fprintf(stderr, "\a");
            }
            continue;
        }

        /* Backspace handling */
        else if (c == 127 || c == '\b') {
            if (bufpos > 0) {
                bufpos--;
                buffer[bufpos] = '\0';
                suggestion[0] = '\0';
                redraw_current_line(buffer);
            }
            continue;
        }

        /* Escape sequence handling (arrow keys, etc.) */
        else if (c == 27) {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            seq[2] = '\0';

            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    /* Up arrow - navigate to previous history entry */
                    if (history_index > 0) {
                        if (history_index == history_count) {
                            buffer[bufpos] = '\0';
                            shell_strlcpy(current_line, buffer, sizeof(current_line));
                        }
                        history_index--;
                        shell_strlcpy(buffer, history[history_index], sizeof(buffer));
                        bufpos = strlen(buffer);
                        suggestion[0] = '\0';
                        redraw_current_line(buffer);
                    }
                } else if (seq[1] == 'B') {
                    /* Down arrow - navigate to next history entry */
                    if (history_index < history_count) {
                        history_index++;
                        if (history_index == history_count) {
                            shell_strlcpy(buffer, current_line, sizeof(buffer));
                        } else {
                            shell_strlcpy(buffer, history[history_index], sizeof(buffer));
                        }
                        bufpos = strlen(buffer);
                        suggestion[0] = '\0';
                        redraw_current_line(buffer);
                    }
                } else if (seq[1] == 'C') {
                    /* Right arrow accepts current suggestion */
                    if (suggestion[0]) {
                        size_t suggestion_len = strlen(suggestion);
                        if (bufpos + suggestion_len < (int)sizeof(buffer)) {
                            strcpy(buffer + bufpos, suggestion);
                            bufpos += suggestion_len;
                            buffer[bufpos] = '\0';
                        }
                        suggestion[0] = '\0';
                        redraw_current_line(buffer);
                    }
                }
            }
            continue;
        }

        /* Regular printable character */
        else if (c >= 32 && c <= 126) {
            if (bufpos < (int)sizeof(buffer) - 1) {
                buffer[bufpos++] = c;
                buffer[bufpos] = '\0';
                suggestion[0] = '\0';
                fprintf(stderr, "%c", c);
                char *history_sugg = find_history_suggestion(buffer);
                if (history_sugg) {
                    shell_strlcpy(suggestion, history_sugg, sizeof(suggestion));
                    fprintf(stderr, "\033[90m%s\033[0m", suggestion);
                    fprintf(stderr, "\033[K");
                }
            } else {
                SHELL_WARN("command line too long");
            }
            continue;
        }
    }
#endif
}


int parse_and_execute(struct source_s *src)
{
    skip_white_spaces(src);

    struct token_s *tok = tokenize(src);

    if(tok == &eof_token)
    {
        return 0;
    }

    while(tok && tok != &eof_token)
    {
        struct node_s *cmd = parse_simple_command(tok);

        if(!cmd)
        {
            break;
        }

        do_simple_command(cmd);
        free_node_tree(cmd);
        tok = tokenize(src);
    }
    return 1;
}
