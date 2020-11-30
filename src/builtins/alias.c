/*
    psh/backends/posix2/run.c - process-related posix backend
    Copyright 2020 Manuel Bertele

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

#include "backend.h"
#include "builtin.h"
#include "libpsh/util.h"
#include "libpsh/xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char ***aliases = NULL;

static int alias_amount = 0;

int add_alias(char *alias, char *value)
{

    if (aliases == NULL)
        aliases = xmalloc(sizeof(char *));
    else
    {
        xrealloc(aliases, (alias_amount + 1) * sizeof(char *));
    }

    aliases[alias_amount] = calloc(2, sizeof(char *));
    aliases[alias_amount][0] = xmalloc(strlen(alias));
    aliases[alias_amount][1] = xmalloc(strlen(value));
    strcpy(aliases[alias_amount][0], alias);
    strcpy(aliases[alias_amount][1], value);

    alias_amount++;

    return 0;
}

char *check_for_alias(char *alias)
{
    int i;
    for (i = 0; i < alias_amount; i++)
    {
        if (strcmp(alias, aliases[i][0]) == 0)
        {
            return aliases[i][1];
        }
    }

    return alias;

}



int builtin_alias(int argc, char **argv, psh_state *state)
{

    int i;
    char *alias, *value, *argstr;

    argstr = xmalloc(strlen(argv[1]) + 1);
    strcpy(argstr, argv[1]);

    /* Combine all argv pointers to one pointer */
    for (i = 2; i < argc; i++)
    {
        xrealloc(argstr, strlen(argstr) + strlen(argv[i]) + 1);
        strcat(argstr, argv[i]);
        strcat(argstr, " ");
    }

    /* Split the string at '=' */
    for (i = 0; i < strlen(argstr); i++)
    {
        if (argstr[i] == '=')
        {
            alias = xcalloc(i, sizeof(char));
            memcpy(alias, argstr, i);
            value = xcalloc(strlen(argstr) - i, sizeof(char));
            strcpy(value, argstr + i + 1);
            break;
        }

    }

    free(argstr);

    if (strcmp(alias, check_for_alias(alias)) == 0)
    {
        add_alias(alias, value);
    } else
    {
        OUT2E("Alias %s already exists with %s\n", alias, check_for_alias(alias));
    }

    free(alias);
    free(value);

    return 0;

}


int builtin_unalias(int argc, char **argv, psh_state *state)
{

    puts("hallo");
}


int expand_alias(struct _psh_command *cmd)
{


}
