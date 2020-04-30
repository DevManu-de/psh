/*
   builtin/builtin.c - builtin command builtin

   Copyright 2017-2020 Zhang Maiyun. 

   This file is part of Psh, P shell.

   Psh is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Psh is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "builtin.h"

int builtin_builtin(ARGS)
{
    if (bltin_argv[1] == NULL) /* No args */
        return 1;
    info->parameters++; /* Skip command name([0]) */
    if (run_builtin(info) == 0)
    {
        info->parameters--;
        OUT2E("%s: %s: %s: not a shell builtin\n", argv0, info->parameters[0],
              info->parameters[1]);
        return 2;
    }
    info->parameters--;
    return 1;
}
