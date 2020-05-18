/*
    prompts.c - Prompt generator and printer
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
/* `$PS1` escape sequences
\a	bell (ascii 07)
\d	the date in Day Mon Date format
\e	escape (ascii 033)
\h	the hostname up to the first `.'
\H	the hostname
\j	the number of active jobs
\l	the basename of the shell's tty device name
\n	CRLF
\r	CR
\s	the name of the shell
\t	the time in 24-hour hh:mm:ss format
\T	the time in 12-hour hh:mm:ss format
\@	the time in 12-hour hh:mm am/pm format
\A	the time in 24-hour hh:mm format
\D{fmt}	the result of passing FMT to strftime(3)
\u	your username
\v	the version of sh (e.g., 2.00)
\V	the release of sh, version + patchlevel (e.g., 2.00.0)
\w	the current working directory
\W	the last element of $PWD
\!	the history number of this command
\#	the command number of this command
\$	a $ or a # if you are root
\nnn	character code nnn in octal
\\	a backslash
\[	begin a sequence of non-printing chars
\]	end a sequence of non-printing chars
*/

#include <ctype.h>
#include <string.h>
#include <time.h>

#include "backend.h"
#include "libpsh/stringbuilder.h"
#include "libpsh/util.h"
#include "libpsh/xmalloc.h"
#include "pshell.h"

static char *workdir_expander(int if_last_component)
{
    /* Actually $PWD should be used here
     * But changing $PWD really isn't so meaningful
     * so let pshgetcwd_dm decide it.
     */
    size_t count, lenhome;
    char *last_occur, *home = gethd(), *pathname = pshgetcwd_dm();

    /* Replace $HOME with a tlide */
    for (count = 0, lenhome = strlen(home); count < lenhome; ++count)
    {
        if (home[count] != pathname[count])
        {
            /* Doesn't match, isn't under home. Set a marker. */
            count = lenhome + 1;
            break;
        }
    }
    /* Matches */
    if (count <= lenhome)
    {
        pathname[0] = '~';
        /* pathname[lenhome] == '/', so the leading slash got kept.
         * if pathname is the same as home, strlen(pathname) - lenhome == 0,
         * only '0' gets moved
         */
        memmove(pathname + 1, pathname + lenhome,
                strlen(pathname) + 1 /* move '0' also */ - lenhome);
    }
    if (if_last_component)
    {
        last_occur = strrchr(pathname, '/');
        if (last_occur && last_occur != pathname)
        {
            size_t diff = last_occur - pathname;
            /* Next line: first +1 to jump over '/', second +1 for '0', -1 for
             * '/' again */
            memmove(pathname, last_occur + 1, strlen(pathname) + 1 - 1 - diff);
        }
        /* if last_occur == pathname: only '/' is left, keep it */
    }
    return pathname;
}

/* Expands $PS1-4, result needs to be free()d */
char *ps_expander(char *prompt)
{
/* cur: current char in process
 * start: start of current piece of string
 * reset_start: reset start of current string
 */
#define reset_start(newloc) (start = (newloc) + 1, cur = (newloc), count = 0)
/* replace_char: replace "\\x" with another character */
#define replace_char(newch)                                                    \
    ((*(cur - 1) /* the '\\' */ = (newch)),                                    \
     psh_stringbuilder_add_length(builder, start, count, 0))
    char *result;
    /* Our approach alters the original PROMPT, so duplicate it */
    char *start = psh_strdup(prompt);
    char *save_start = start;
    char *cur = start;
    /* Count of current number of characters to write */
    int count = 0;
    /* Whether the last processed character is '\\' */
    int is_backslash = 0;
    /* When \nnn is encountered, this records the number of integer characters
     * got */
    int num_level = 0;
    psh_stringbuilder *builder = psh_stringbuilder_create();

    do
    {
        /* This branching only handles COUNT */
        if (*cur != '\\') /* likely */
            ++count;
        switch (*cur)
        {
            /* XXX: Should this be ordered by freq? */
            case '\\':
                if (is_backslash)
                {
                    /* A backslash was before this backslash */
                    /* Add one of them to the builder */
                    /* Performance will be a issue if people use multiple
                     * '\\\\'s */
                    is_backslash = 0;
                    psh_stringbuilder_add_length(builder, start,
                                                 count + 1 /* for '/' */, 0);
                    reset_start(cur);
                }
                else
                    is_backslash = 1;
                break;
            case '$':
                if (is_backslash)
                {
                    is_backslash = 0;
                    if (pshgetuid() == 0) /* root */
                        replace_char('#');
                    else
                        replace_char('$');
                    reset_start(cur);
                }
                /* else write $ */
                break;
            case 'a':
                if (is_backslash)
                {
                    is_backslash = 0;
                    replace_char('\a');
                    reset_start(cur);
                }
                /* else write a */
                break;
            case 'd':
                if (is_backslash)
                {
                    char *timestr = xmalloc(P_CS * 11);
                    time_t rt;
                    struct tm *ti;
                    is_backslash = 0;

                    time(&rt);
                    ti = localtime(&rt);
                    strftime(timestr, 11, "%a %b %e", ti);
                    /* Strftime()'s %e prepends a space to single digits, but we
                     * want a '0' */
                    if (timestr[8] == ' ' /* space */)
                        timestr[8] = '0';

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, timestr, 1);
                    reset_start(cur);
                }
                /* else write d */
                break;
            case 'e':
                if (is_backslash)
                {
                    is_backslash = 0;
                    replace_char('\033');
                    reset_start(cur);
                }
                /* else write e */
                break;
            case 'h':
                if (is_backslash)
                {
                    char *dot, *hostname = pshgethostname_dm();
                    is_backslash = 0;

                    dot = strchr(hostname, '.');
                    if (dot)
                        *dot = 0;

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, hostname, 1);
                    reset_start(cur);
                }
                /* else write h */
                break;
            case 'H':
                if (is_backslash)
                {
                    char *hostname = pshgethostname_dm();
                    is_backslash = 0;

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, hostname, 1);
                    reset_start(cur);
                }
                /* else write H */
                break;
            case 'n':
                if (is_backslash)
                {
                    is_backslash = 0;
                    replace_char('\n');
                    reset_start(cur);
                }
                /* else write n */
                break;
            case 'r':
                if (is_backslash)
                {
                    is_backslash = 0;
                    replace_char('\r');
                    reset_start(cur);
                }
                /* else write r */
                break;
            case 'u':
                if (is_backslash)
                {
                    is_backslash = 0;

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, getun(), 0);
                    reset_start(cur);
                }
                /* else write u */
                break;
            case '[':
                if (is_backslash)
                {
                    is_backslash = 0;
#ifdef NO_READLINE
                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
#else
                    replace_char('\001'); /* RL_PROMPT_START_IGNORE */
#endif
                    reset_start(cur);
                }
                /* else write [ */
                break;
            case ']':
                if (is_backslash)
                {
                    is_backslash = 0;
#ifdef NO_READLINE
                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
#else
                    replace_char('\002'); /* RL_PROMPT_END_IGNORE */
#endif
                    reset_start(cur);
                }
                /* else write ] */
                break;
            case 'w':
                if (is_backslash)
                {
                    char *pathname = workdir_expander(0);
                    is_backslash = 0;

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, pathname, 1);
                    reset_start(cur);
                }
                /* else write w */
                break;
            case 'W':
                if (is_backslash)
                {
                    is_backslash = 0;
                    char *pathname = workdir_expander(1);

                    psh_stringbuilder_add_length(builder, start, count - 1, 0);
                    psh_stringbuilder_add(builder, pathname, 1);
                    reset_start(cur);
                }
                /* else write W */
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                if (num_level || is_backslash)
                {
                    --count; /* These numbers shouldn't be added */
                    is_backslash = 0;
                    ++num_level;
                    /* Now *(cur-num_level) is always '\\' */
                    if (num_level == 3 || !isdigit(*(cur + 1)))
                    {
                        char save = *(cur + 1);
                        int ch;
                        /* Manually mark an end to the string */
                        *(cur + 1) = 0;
                        sscanf(cur - num_level + 1, "%o", &ch);
                        /* Restore the old character (possibly NUL) */
                        *(cur + 1) = save;
                        /* Replace '\\' with the new char */
                        *(cur - num_level) = ch;
                        /* Reset num_level as the current one is processed */
                        num_level = 0;
                        /* count wasn't increased since '\\', so +1 for the new
                         * char */
                        psh_stringbuilder_add_length(builder, start, count + 1,
                                                     0);
                        reset_start(cur);
                    }
                }
                /* else write [0-9] */
                break;
            case 'j':
            case 'l':
            case 's':
            case 't':
            case 'T':
            case '@':
            case 'A':
            case 'D':
            case 'v':
            case 'V':
            case '!':
            case '#':
            default:
                /* When the escape is unknown, bash and dash keeps both
                   the backslash and the character. */
                if (is_backslash)
                {
                    /* For '\\' */
                    ++count;
                    is_backslash = 0;
                }
                break;
        }
    } while (*(++cur) /* when *cur == 0, exit */);
    psh_stringbuilder_add(builder, start, 0);
    result = psh_stringbuilder_yield(builder);
#if DEBUG
    {
        struct _psh_sb_item *cur = builder->first;
        while (cur)
        {
            printf("Len: %zu str: %s\n", cur->length, cur->string);
            cur = cur->next;
        }
    }
#endif
    psh_stringbuilder_free(builder);
    xfree(save_start);
    return result;
}