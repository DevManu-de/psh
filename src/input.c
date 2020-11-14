/*
    psh/input.c - input handler
    Copyright 2020 Zhang Maiyun.

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
/* Some evil implementations include no stdio.h is history.h */
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include "libpsh/util.h"
#include "libpsh/xmalloc.h"
#include "util.h"

extern int last_command_status;
extern char *argv0;

/* read command line, shows PROMPT and result goes into *RESULT
 * *RESULT needs to be free()d
 * returns 0 if everything goes well;
 * and -1 if the cmd doesn't need to be run;
 * and -2 if anything went wrong, RESULT is untouched.
 */
int read_cmdline(char *prompt, char **result)
{
    char *buffer;
#ifdef HAVE_WORKING_HISTORY
    char *expanded;
    int stat;
#endif

    buffer = psh_gets(prompt);
    if (!buffer) /* EOF reached */
    {
        puts("");
        exit_psh(last_command_status);
    }
    if (*buffer == 0)
    {
        xfree(buffer);
        return -1;
    }
#ifdef HAVE_WORKING_HISTORY
    stat = history_expand(buffer, &expanded);
    if (stat < 0)
    {
        OUT2E("%s: Error on history expansion: %s\n", argv0, expanded);
        xfree(expanded);
        return -2;
    }
    if (stat == 1 || stat == 2)
        printf("%s\n", expanded);
    if (stat == 2)
    {
        /* cmd need not run */
        xfree(expanded);
        return -1;
    }
    xfree(buffer);
    add_history(expanded);
    buffer = expanded;
#endif /* HAVE_WORKING_HISTORY */
    *result = buffer;
    return 0;
}
