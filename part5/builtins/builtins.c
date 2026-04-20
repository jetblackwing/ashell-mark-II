/* 
 *    Programmed By: Mohammed Isam Mohammed [mohammed_isam1984@yahoo.com]
 *    Copyright 2020 (c)
 *    Updated with additional builtins
 * 
 *    file: builtins.c
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

#include "../shell.h"
#include "shell_builtins.h"

struct builtin_s builtins[] =
{
    { "dump"    , dump              },
    { "cd"      , shell_cd          },
    { "pwd"     , shell_pwd         },
    { "export"  , shell_export      },
    { "unset"   , shell_unset       },
    { "echo"    , shell_echo        },
    { "type"    , shell_type        },
};

int builtins_count = sizeof(builtins)/sizeof(struct builtin_s);
