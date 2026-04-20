/* 
 *    Programmed By: Mohammed Isam [mohammed_isam1984@yahoo.com]
 *    Copyright 2020 (c)
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

    char buffer[MAX_INPUT_BUFFER];
    char current_line[MAX_INPUT_BUFFER] = {0};
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
        
        /* Backspace handling */
        else if (c == 127 || c == '\b') {
            if (bufpos > 0) {
                bufpos--;
                fprintf(stderr, "\b \b");
            }
        }
        
        /* Escape sequence handling (arrow keys, etc.) */
        else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                /* Up arrow - navigate to previous history entry */
                if (seq[1] == 'A') {
                    if (history_index > 0) {
                        if (history_index == history_count) {
                            buffer[bufpos] = '\0';
                            shell_strlcpy(current_line, buffer, sizeof(current_line));
                        }
                        history_index--;
                        
                        /* Clear current line on display */
                        for (int i = 0; i < bufpos; i++) {
                            fprintf(stderr, "\b \b");
                        }
                        
                        /* Copy history entry to buffer */
                        shell_strlcpy(buffer, history[history_index], sizeof(buffer));
                        bufpos = strlen(buffer);
                        fprintf(stderr, "%s", buffer);
                    }
                }
                /* Down arrow - navigate to next history entry */
                else if (seq[1] == 'B') {
                    if (history_index < history_count) {
                        history_index++;
                        
                        /* Clear current line on display */
                        for (int i = 0; i < bufpos; i++) {
                            fprintf(stderr, "\b \b");
                        }
                        
                        /* Show history entry or restore current line */
                        if (history_index == history_count) {
                            shell_strlcpy(buffer, current_line, sizeof(buffer));
                        } else {
                            shell_strlcpy(buffer, history[history_index], sizeof(buffer));
                        }
                        bufpos = strlen(buffer);
                        fprintf(stderr, "%s", buffer);
                    }
                }
            }
        }
        
        /* Regular printable character */
        else if (c >= 32 && c <= 126) {
            if (bufpos < (int)sizeof(buffer) - 1) {
                buffer[bufpos++] = c;
                fprintf(stderr, "%c", c);
            } else {
                SHELL_WARN("command line too long");
            }
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
