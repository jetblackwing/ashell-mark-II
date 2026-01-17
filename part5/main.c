/* 
 *    Programmed By: Mohammed Isam [mohammed_isam1984@yahoo.com]
 *    Copyright 2020 (c)
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

#define HISTORY_SIZE 100

// History Implementation. Experimental
char *history[HISTORY_SIZE];
int history_count = 0;


void add_to_history(char *cmd) {
    char *line = strdup(cmd);
    if (line[strlen(line)-1] == '\n') {
        line[strlen(line)-1] = '\0';
    }
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = line;
    } else {
        free(history[0]);
        memmove(history, history + 1, (HISTORY_SIZE - 1) * sizeof(char*));
        history[HISTORY_SIZE - 1] = line;
    }
}


int main(int argc, char **argv)
{
    char *cmd;

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


char *read_cmd(void)
{
#ifdef _WIN32
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return NULL;
    }
    return strdup(buffer);
#else
    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    char buffer[1024];
    char current_line[1024] = {0};
    int bufpos = 0;
    int history_index = history_count;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            return NULL;
        }
        if (c == '\n') {
            buffer[bufpos] = '\0';
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            fprintf(stderr, "\n");
            char *ret = malloc(bufpos + 2);
            strcpy(ret, buffer);
            strcat(ret, "\n");
            return ret;
        } else if (c == 127 || c == '\b') { // backspace
            if (bufpos > 0) {
                bufpos--;
                fprintf(stderr, "\b \b");
            }
        } else if (c == 27) { // escape sequence
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // up arrow
                    if (history_index > 0) {
                        if (history_index == history_count) {
                            buffer[bufpos] = '\0';
                            strcpy(current_line, buffer);
                        }
                        history_index--;
                        // clear current line
                        for (int i = 0; i < bufpos; i++) fprintf(stderr, "\b \b");
                        strcpy(buffer, history[history_index]);
                        bufpos = strlen(buffer);
                        fprintf(stderr, "%s", buffer);
                    }
                } else if (seq[1] == 'B') { // down arrow
                    if (history_index < history_count) {
                        history_index++;
                        for (int i = 0; i < bufpos; i++) fprintf(stderr, "\b \b");
                        if (history_index == history_count) {
                            strcpy(buffer, current_line);
                        } else {
                            strcpy(buffer, history[history_index]);
                        }
                        bufpos = strlen(buffer);
                        fprintf(stderr, "%s", buffer);
                    }
                }
            }
        } else if (c >= 32 && c <= 126) { // printable
            if (bufpos < 1023) {
                buffer[bufpos++] = c;
                fprintf(stderr, "%c", c);
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
