/*
    psh/builtins/true.c - builtin true
    Copyright 2018 Zhang Maiyun.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "builtin.h"
#include "psh.h"

int builtin_true(ATTRIB_UNUSED int argc, ATTRIB_UNUSED char **argv,
                 ATTRIB_UNUSED psh_state *state)
{
    return 0;
}

int builtin_false(ATTRIB_UNUSED int argc, ATTRIB_UNUSED char **argv,
                  ATTRIB_UNUSED psh_state *state)
{
    return 1;
}
