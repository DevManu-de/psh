/*
    psh/builtins/history.c - builtin history
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtin.h"
#include "libpsh/util.h"
#include "libpsh/xmalloc.h"

#define USAGE()                                                                \
    OUT2E("history: usage: history [-c] [-d offset] [n] or history -awrn "     \
          "[filename] or history -ps arg [arg...]\n")
#define AFLAG 0x01
#define RFLAG 0x02
#define WFLAG 0x04
#define NFLAG 0x08
#define SFLAG 0x10
#define PFLAG 0x20
#define CFLAG 0x40
#define DFLAG 0x80

int builtin_history(int argc, char **argv)
{
#ifdef NO_HISTORY
    OUT2E("%s: libhistory not compiled!\n", argv[0]);
    return 1;
#else
    if (argv[1] != NULL)
    {
        int count, ch, flags = 0, n;
        char *filename = xmalloc(P_CS * MAXEACHARG);
        struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                                    {NULL, 0, NULL, 0}};
        
        while ((ch = getopt_long(argc, argv, ":a::w::r::n::p::s::cd:", longopts,
                                 NULL)) != -1)
        {
            switch (ch)
            {
                case 'a':
                    flags |= AFLAG;
                    if (optarg)
                        psh_strncpy(filename, optarg,
                                    sizeof(char) * MAXEACHARG - 1);
                    break;
                case 'r':
                    if (flags & AFLAG)
                    {
                        OUT2E("%s: %s: cannot use more "
                              "than one of -anrw\n",
                              argv0, argv[0]);
                        USAGE();
                        free(filename);
                        return 2;
                    }
                    flags |= RFLAG;
                    if (optarg)
                        psh_strncpy(filename, optarg,
                                    sizeof(char) * MAXEACHARG - 1);
                    break;
                case 'w':
                    if (flags & AFLAG || flags & RFLAG)
                    {
                        OUT2E("%s: %s: cannot use more "
                              "than one of -anrw\n",
                              argv0, argv[0]);
                        USAGE();
                        free(filename);
                        return 2;
                    }
                    if (optarg)
                        psh_strncpy(filename, optarg,
                                    sizeof(char) * MAXEACHARG - 1);
                    flags |= WFLAG;
                    break;
                case 'n':
                    if (flags & AFLAG || flags & RFLAG || flags & WFLAG)
                    {
                        OUT2E("%s: %s: cannot use more "
                              "than one of -anrw\n",
                              argv0, argv[0]);
                        USAGE();
                        free(filename);
                        return 2;
                    }
                    if (optarg)
                        psh_strncpy(filename, optarg,
                                    sizeof(char) * MAXEACHARG - 1);
                    flags |= NFLAG;
                    break;
                case 's':
                    flags |= SFLAG;
                    break;
                case 'p':
                    flags |= PFLAG;
                    break;
                case 'c':
                    flags |= CFLAG;
                    break;
                case 'd':
                    flags |= DFLAG;
                    n = atoi(argv[count]);
                    if (n < 0)
                    {
                        OUT2E("%s: %s: %d: invalid "
                              "option\n",
                              argv0, argv[0], n);
                        free(filename);
                        return 2;
                    }
                    if (!n)
                    {
                        int count2;
                        for (count2 = 0; argv[count][count2]; ++count2)
                            if (argv[count][count2] != '0' &&
                                (!isspace(argv[count][count2])))
                            {
                                OUT2E("%s: %s: "
                                      "%s: "
                                      "numeric "
                                      "argument "
                                      "required"
                                      "\n",
                                      argv0, argv[0], argv[count]);
                                free(filename);
                                return 2;
                            }
                    }
                    break;
                case '?':
                    OUT2E("%s: %s: invalid option '-%c'\n", argv0, argv[0],
                          optopt);
                    free(filename);
                    return 2;
                case ':':
                    OUT2E("%s: %s: -d: option requires an "
                          "argument\n",
                          argv0, argv[0]);
                    free(filename);
                    return 2;
            }
        }
        if (flags == 0)
        {
            free(filename);
            goto noopts;
        }
        else
        {
            /* TODO:Code here */
            free(filename);
        }
    }
    else
    noopts : {
        HIST_ENTRY **histlist;
        int i;

        histlist = history_list();
        if (histlist)
            for (i = 0; histlist[i]; i++)
                printf("    %d  %s\n", i + history_base, histlist[i]->line);
    }
        return 1;
#endif
}
