/*
    psh/variables.c - psh variables
    Copyright 2020 Zhang Maiyun

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

#include <stdio.h>

#include "libpsh/hash.h"
#include "libpsh/util.h"
#include "libpsh/xmalloc.h"
#include "variable.h"

/* Using three tables here, because both env vars and global vars live
 * throughout the life of the shell, while local vars can override them without
 * altering their value. */
/* Environmental variables */
psh_hash *variable_table_e = NULL;
/* Global variables */
psh_hash *variable_table_g = NULL;
/* Local variables */
psh_hash *variable_table_l = NULL;

/* Init the two tables above, and try to get environment parameters */
void psh_variable_init(void)
{
    variable_table_e = psh_hash_create(10);
    variable_table_g = psh_hash_create(10);
    variable_table_l = psh_hash_create(10);
    /* #5 #12 #13 TODO: Retrieve all env vars,
     * for generic, only try to read those important to shell, such as HOME,
     * PATH, etc. */
}

/* Set or update a variable, either name or scope.
 * Moving a variable among scopes should be fast, as they should be only pointer
 * operations. */
int psh_variable_set(const char *varname, const char *value,
                     psh_variable_scope scope)
{
}

/* Get a variable's value, based on scope precedence, i.e. local->global,env */
char *psh_variable_get(const char *varname) {}

/* Clear all local variables. */
void psh_variable_exit_local(void) {}

/* Unset a variable. Removes any matching entry in all three tables. */
void psh_variable_unset(const char *varname) {}

/* Called upon shell exit, destroy the whole variable database */
void psh_variable_destroy(void) {}