/* 
 *    Programmed By: jetblackwing [amaljk80@gmail.com]
 *    Copyright 2026 (c) jetblackwing [amaljk80@gmail.com]
 * 
 *    file: initsh.c
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

#include <string.h>
#include "shell.h"
#include "symtab/symtab.h"
#include <stdio.h>
#include <stdlib.h>

/* Load minimal config from ../config/shell.conf (KEY=VALUE lines)
 * Supported keys: PS1, PS2
 */
static void load_config(void)
{
    const char *path = "../config/shell.conf";
    FILE *f = fopen(path, "r");
    if (!f) {
        return; /* no config - that's fine */
    }

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        /* trim leading spaces */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n' || *p == '#') continue;

        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = p;
        char *val = eq + 1;

        /* strip trailing newline from val */
        char *nl = strchr(val, '\n');
        if (nl) *nl = '\0';

        /* add or update symbol table entry */
        struct symtab_entry_s *entry = add_to_symtab(key);
        if (entry) {
            /* make a copy for symtab storage */
            char *vcopy = strdup(val);
            if (vcopy) symtab_entry_setval(entry, vcopy);
        }
    }

    fclose(f);
}

extern char **environ;

void initsh()
{
    init_symtab();

    struct symtab_entry_s *entry;
    char **p2 = environ;
    
    while(*p2)
    {
        char *eq = strchr(*p2, '=');
        if(eq)
        {
            int len = eq-(*p2);
            char name[len+1];
            
	    strncpy(name, *p2, len);
            name[len] = '\0';
            entry = add_to_symtab(name);
            
	    if(entry)
            {
                symtab_entry_setval(entry, eq+1);
                entry->flags |= FLAG_EXPORT;
            }
        }
        else
        {
            entry = add_to_symtab(*p2);
        }
        p2++;
    }

    
    entry = add_to_symtab("PS1");
    symtab_entry_setval(entry, "$ ");

    entry = add_to_symtab("PS2");
    symtab_entry_setval(entry, "> ");

    /* load user config (if present) */
    load_config();
}
