/* 
 *    Programmed By: jetblackwing [amaljk80@gmail.com]
 *    Copyright 2026 (c) jetblackwing [amaljk80@gmail.com]
 * 
 *    file: executor.h
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

#ifndef BACKEND_H
#define BACKEND_H

#include "node.h"

/**
 * Search for a command file in the PATH environment variable
 * @param file - Command name to search for
 * @return Full path to executable, or NULL if not found
 */
char *search_path(const char *file);

/**
 * Execute a command after it's been found
 * @param argc - Argument count
 * @param argv - Argument vector
 * @return 0 on error (after exec), or 1 on fork success
 */
int do_exec_cmd(int argc, char **argv);

/**
 * Execute a simple command (possibly with redirections)
 * @param node - AST node representing the command
 * @return 1 on success, 0 on failure
 */
int do_simple_command(struct node_s *node);

#endif
