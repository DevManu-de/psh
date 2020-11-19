/*
    psh/util.c - psh-specific utilities
    Copyright 2018-2020 Zhang Maiyun

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

#include <stdint.h>
#include <stdlib.h>

#include "libpsh/hash.h"
#include "libpsh/util.h"
#include "psh.h"
#include "util.h"

/* Some unexpected things happened */
__attribute__((noreturn)) void code_fault(psh_state *state, char *file,
                                          int line)
{
    OUT2E("%s: Programming error at %s: %d\n", state->argv0, file, line);
    OUT2E("Shell version: %s", PSH_VERSION);
    OUT2E("Please create a GitHub Issue with above info\n");
    exit_psh(state, 1);
}

/* Exit psh after cleaning up */
void exit_psh(psh_state *state, int status)
{
    free(state->argv0);
    free(state->signals_pending);
    psh_hash_free(state->command_table);
    free(state);
    exit(status);
}
