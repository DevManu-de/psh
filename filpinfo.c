/*
  filpinfo - function to fill parse info(merges original preprocesser, splitbuf,
  parser) and some other functions for command magaging

   Copyright 2017 Zhang Maiyun.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <ctype.h>
#include "backends/backend.h"
#include "pshell.h"

static void command_init(struct command *info)
{
	info->flag = 0;
	info->rlist = NULL;
	info->parameters = NULL;
	info->next = NULL;
}

static void redirect_init(struct redirect *redir)
{
	redir->in.fd = 0;
	redir->in.file = NULL;
	redir->out.fd = 0;
	redir->out.file = NULL;
	redir->type = 0;
	redir->next = NULL;
}

/* Returns the nth next of the command base, n begins with 1 */
static struct command *getpos(struct command *base, int n)
{
	int count;
	struct command *info = base;
	for (count = 1; count < n; ++count)
		info = info->next;
	return info;
}

static void free_parameters(struct command *info)
{
	int count;
	for (count = 0; count < MAXARG; ++count)
	{
		if (info->parameters[count] != NULL)
		{
			free(info->parameters[count]);
			info->parameters[count] = NULL;
		}
		else
			break; /* All parameters should be freed after here */
	}
	free(info->parameters);
	info->parameters = NULL;
}

static void free_redirect(struct redirect *redir)
{
	struct redirect *temp;
	while (redir != NULL)
	{
		temp = redir;
		redir = redir->next;
		free(temp);
		temp = NULL;
	}
}

/* Get the index of the last char in the buffer */
static int ignore_IFSs(char *buffer, int count)
{
	do
	{
		if (!buffer[count]) /* EOL */
			return -5;
		if (buffer[count] != ' ' && buffer[count] != '\t')
			return --count;
	} while (++count);
	return -6; /* Reaching here impossible */
}

/* Malloc a command, enNULL all elements, malloc the first parameter[] and a
 * struct redirect */
int new_command(struct command **info)
{
	*info = malloc(sizeof(struct command));
	if ((*info) == NULL)
		return -1;
	command_init(*info);
	(*info)->parameters = malloc(sizeof(char *) * MAXARG);
	if ((*info)->parameters == NULL)
	{
		free(*info);
		return -1;
	}
	memset((*info)->parameters, 0,
	       MAXARG); /* This will be used to detect whether an element is
			   used */
	(*info)->parameters[0] = malloc(sizeof(char) * MAXEACHARG);
	if ((*info)->parameters[0] == NULL)
	{
		free((*info)->parameters);
		free(*info);
		return -1;
	}
	memset((*info)->parameters[0], 0, MAXEACHARG);
	(*info)->rlist = malloc(sizeof(struct redirect));
	redirect_init((*info)->rlist);
	return 0;
}

/* Free a command and its nexts */
void free_command(struct command *info)
{
	struct command *temp;
	while (info != NULL)
	{
		temp = info;
		info = info->next;
		free_parameters(temp);
		free_redirect(temp->rlist);
		free(temp);
		temp = NULL;
	}
}

/* Malloc and fill a command with a buffer, free() buffer, return characters
 * processed */
int filpinfo(char *buffer, struct command *info)
{
#define synerr(token)                                                          \
	OUT2E("%s: syntax error near unexpected token `%s'\n", argv0, token)
#define CUR_INFO getpos(info, cnt_strct_cmd)

#define ignIFS()                                                               \
	do                                                                     \
	{                                                                      \
		int tmp;                                                       \
		if ((tmp = ignore_IFSs(buffer, cnt_buffer)) == -5)             \
			goto done;                                             \
		cnt_buffer = tmp;                                              \
	} while (0)

#define ignIFS_from_next_char()                                                \
	do                                                                     \
	{                                                                      \
		int tmp;                                                       \
		if ((tmp = ignore_IFSs(buffer, ++cnt_buffer)) == -5)           \
			goto done;                                             \
		cnt_buffer = tmp;                                              \
	} while (0)

#define malloc_one(n)                                                          \
	(getpos(info, cnt_strct_cmd)->parameters[n]) =                         \
	    malloc(sizeof(char) * MAXEACHARG);                                 \
	memset(getpos(info, cnt_strct_cmd)->parameters[n], 0, MAXEACHARG)

/* Write the current char in buffer to current command, increase cnt_return
 * only if current not blank or 0 */
#define write_current()                                                        \
	do                                                                     \
	{                                                                      \
		getpos(info, cnt_strct_cmd)                                    \
		    ->parameters[cnt_argument_element][cnt_argument_char++] =  \
		    buffer[cnt_buffer];                                        \
		if (strchr(" \t", buffer[cnt_buffer]) == NULL &&               \
		    buffer[cnt_buffer] != 0) /* current char not blank */      \
			cnt_return++;                                          \
	} /* Make the semicolon happy */ while (0)

/* Write any char to current command, increase cnt_return only if c != 0 */
#define write_char(c)                                                          \
	do                                                                     \
	{                                                                      \
		getpos(info, cnt_strct_cmd)                                    \
		    ->parameters[cnt_argument_element][cnt_argument_char++] =  \
		    c;                                                         \
		if (strchr(" \t", c) == NULL && c != 0)                        \
			cnt_return++;                                          \
	} while (0)

#define escape (cnt_buffer != 0 && buffer[cnt_buffer - 1] == '\\')
#define ignore (stat_in_dquote == 1 || stat_in_squote == 1 || escape)
	/*
		escape: determine whether the last character is '\\'
		ignore: determine whether a meta character should be ignored(not
	   for a dollar)
	*/

	int len = strlen(buffer);
	int stat_in_squote = 0, stat_in_dquote = 0;
	int cnt_strct_cmd = 1;
	int cnt_buffer = 0, cnt_argument_char = 0, cnt_argument_element = 0,
	    cnt_return = 0, cnt_old_parameter = 0, cnt_first_nonIFS = 0;
	/*
		Variable prefixes:
			cnt: count;
			stat: status
		Variable description:
			stat_in_squote: whether in a '' quote
			stat_in_dquote: whether in a "" quote

			cnt_strct_cmd: count representing which node of the
	   struct command list we are currently on
			cnt_buffer: count for buffer
			cnt_argument_char: count for current parameter element
			cnt_argument_element: count representing how many elements are
	   there in parameter cnt_return: characters actually wrote to the
	   command, returned cnt_old_parameter: saved cnt_argument_char for undo
	   IFS cnt_first_nonIFS: the first non-IFS char in buffer
	*/
	if (info == NULL)
	{
		OUT2E("%s: filpinfo: info is NULL\n", argv0);
		return -1;
	}
	ignIFS();
	cnt_first_nonIFS = ++cnt_buffer;
	/* The input command should be initialized */
	for (; cnt_buffer < len; ++cnt_buffer)
	{
		switch (buffer[cnt_buffer])
		{
			case '\'':
				if (stat_in_dquote ==
				    1) /* Already in a "" quote, just write a '
					*/
					write_current();

				else if (stat_in_squote ==
					 1) /* Get out of the quote */
					stat_in_squote = 0;
				else
				{
					if (cnt_buffer == cnt_first_nonIFS)
						stat_in_squote = 1;
					else if (!escape)
						stat_in_squote = 1;
					else /* cnt_buffer != cnt_first_nonIFS
						&& buffer[cnt_buffer-1] ==
						'\\' */
						/* Write a ' */
						write_current();
				}
				break;
			case '"':
				if (stat_in_squote == 1)
					write_current();
				else
				{
					if (stat_in_dquote == 1)
						if (escape)
							write_current();
						else
							stat_in_dquote = 0;
					else if (escape)
						write_current();
					else
						stat_in_dquote = 1;
				}
				break;
			case '\t':
			case ' ':
				if (ignore)
					write_current();
				else
				{
					ignIFS();
					write_char(0);
					cnt_argument_element++;
					cnt_old_parameter = cnt_argument_char;
					cnt_argument_char = 0;
					malloc_one(cnt_argument_element);
				}
				break;
			case '&':
				if (ignore)
					write_current();
				else
				{
					if (cnt_argument_char ==
					    0) /* Previously a blank reached */
					{
						cnt_argument_char =
						    cnt_old_parameter;
						free(
						    CUR_INFO->parameters
							[cnt_argument_element]);
						CUR_INFO->parameters
						    [cnt_argument_element] =
						    NULL;
						cnt_argument_element--;
					}
					if (ignore_IFSs(
						buffer,
						cnt_buffer +
						    1 /* the char after & */) ==
					    -5) /* EOL */
					{
						/* done */
						if (info->flag == 0)
							info->flag =
							    BG_CMD; /* cmd & \0
								     */
						else
						{
							synerr("&");
							cnt_return = -2;
						}
						goto done;
					}
					else if (buffer[cnt_buffer + 1] == '&')
					{
						if (new_command(&(
							CUR_INFO->next)) == -1)
						{
							OUT2E("%s: filpinfo: "
							      "malloc failed\n",
							      argv0);
							cnt_return = -1;
							goto done;
						}
						if (info->flag == 0)
							info->flag = RUN_AND;
						else
						{
							synerr("&&");
							cnt_return = -2;
							goto done;
						}
						if (ignore_IFSs(
							buffer,
							cnt_buffer +
							    2 /* the char after || */) ==
						    -5) /* EOL */
						{
							char *cmdand_buf;
#ifdef NO_READLINE
							cmdand_buf =
							    malloc(MAXLINE);
							printf("> ");
							fgets(cmdand_buf,
							      MAXLINE, stdin);
#else
							cmdand_buf =
							    readline("> ");
							buffer = realloc(
							    buffer,
							    strlen(buffer) +
								strlen(
								    cmdand_buf) +
								1 /* \0 */);
#endif
							strncat(
							    buffer, cmdand_buf,
							    MAXLINE -
								cnt_buffer - 1);
							free(cmdand_buf);
						}
						++cnt_buffer;
					}
					else
					{
						if (new_command(&(
							CUR_INFO->next)) == -1)
						{
							/* malloc failed,
							 * cleanup */
							OUT2E("%s: filpinfo: "
							      "malloc failed\n",
							      argv0);
							cnt_return = -1;
							goto done;
						}
						if (info->flag == 0)
							info->flag = BG_CMD;
						else
						{
							synerr("&");
							cnt_return = -2;
							goto done;
						}
					}
					cnt_strct_cmd++;
					cnt_argument_element = 0;
					cnt_argument_char = 0;
					ignIFS_from_next_char();
				}
				break;
			case '|':
				if (ignore)
					write_current();
				else
				{
					if (cnt_argument_char ==
					    0) /* Previously a blank reached */
					{
						cnt_argument_char =
						    cnt_old_parameter;
						free(
						    CUR_INFO->parameters
							[cnt_argument_element]);
						CUR_INFO->parameters
						    [cnt_argument_element] =
						    NULL;
					}
					if (new_command(&(CUR_INFO->next)) ==
					    -1)
					{
						/* malloc failed, cleanup */
						OUT2E("%s: filpinfo: malloc "
						      "failed\n",
						      argv0);
						cnt_return = -1;
						goto done;
					}
					if (buffer[cnt_buffer + 1] == '|')
					{
						if (info->flag == 0)
							info->flag = RUN_OR;
						else
						{
							synerr("||");
							cnt_return = -2;
							goto done;
						}
						if (ignore_IFSs(
							buffer,
							cnt_buffer +
							    2 /* the char after || */) ==
						    -5) /* EOL */
						{
							char *cmdor_buf;
#ifdef NO_READLINE
							cmdor_buf =
							    malloc(MAXLINE);
							printf("> ");
							fgets(cmdor_buf,
							      MAXLINE, stdin);
#else
							cmdor_buf =
							    readline("> ");
							buffer = realloc(
							    buffer,
							    strlen(buffer) +
								strlen(
								    cmdor_buf) +
								1 /* \0 */);
#endif
							strncat(buffer,
								cmdor_buf,
								MAXLINE -
								    cnt_buffer -
								    1) /*\0*/;
							free(cmdor_buf);
						}
						++cnt_buffer;
					}
					else
					{
						if (info->flag == 0)
							info->flag = PIPED;
						else
						{
							synerr("|");
							cnt_return = -2;
							goto done;
						}
						if (ignore_IFSs(
							buffer,
							cnt_buffer +
							    2 /* the char after | */) ==
						    -5) /* EOL */
						{
							char *pipe_buf;
#ifdef NO_READLINE
							pipe_buf =
							    malloc(MAXLINE);
							printf("> ");
							fgets(pipe_buf, MAXLINE,
							      stdin);
#else
							pipe_buf =
							    readline("> ");
							buffer = realloc(
							    buffer,
							    strlen(buffer) +
								strlen(
								    pipe_buf) +
								1 /* \0 */);
#endif
							strncat(
							    buffer, pipe_buf,
							    MAXLINE -
								cnt_buffer - 1);
							free(pipe_buf);
						}
					}
					cnt_strct_cmd++;
					cnt_argument_element = 0;
					cnt_argument_char = 0;
					ignIFS_from_next_char();
				}
				break;
			case '~': /* This feature stable */
				if (ignore)
				{
					write_current();
					break;
				}
				if (buffer[cnt_buffer + 1] != 0 &&
				    buffer[cnt_buffer + 1] != '\n' &&
				    buffer[cnt_buffer + 1] != '\t' &&
				    buffer[cnt_buffer + 1] != ' ' &&
				    buffer[cnt_buffer + 1] !=
					'/') /* ~username */
				{
					char *username =
					    malloc(sizeof(char) * 256);
					char *posit;
					strncpy(username,
						&(buffer[cnt_buffer + 1]), 256);
					posit = strchr(username, '/');
					if (posit != NULL)
					{
						while (
						    --posit !=
						    username) /* Remove blank */
							if (*posit != ' ' &&
							    *posit != '\t')
							{
								*(++posit) =
								    0; /* Terminate
									  the
									  string
									*/
								break;
							}
					}
					else
					{
						int usernamelen =
						    strlen(username);
						for (--usernamelen;
						     usernamelen != 0;
						     --usernamelen)
							if (username[usernamelen] !=
								' ' &&
							    username[usernamelen] !=
								'\t')
							{
								username
								    [++usernamelen] =
									0; /* Terminate
									      the
									      string*/
								break;
							}
					}
					char *hdir = gethdnam(username);
					if (hdir == NULL)
					{
						/* No such user, treat as a
						 * normal ~ as in bash */
						write_current();
						break;
					}
					strncpy(info->parameters
						    [cnt_argument_element],
						hdir, 4094 - cnt_argument_char);
					cnt_buffer += strlen(username);
					cnt_argument_char += strlen(hdir);
					free(username);
				}
				else /* ~/ and ~ */
				{
					char *hdir = gethd();
					strncpy(info->parameters
						    [cnt_argument_element],
						hdir, 4094 - cnt_argument_char);
					cnt_argument_char += strlen(hdir);
				}
				break;
			case '\\':
			{
				int case_count;
				for (case_count = 1; buffer[cnt_buffer];
				     ++case_count, ++cnt_buffer)
				{
					if (buffer[cnt_buffer] != '\\')
					{
						--cnt_buffer;
						break;
					}
					else /* Print the '\' at a even
						location, and ignore the odd
						ones, the same behavior as in
						bash */
					{
						if (case_count &
						    1) /* Odd number */
							continue;
						else /* Even number */
							write_current();
					}
				}
			}
			case '`':
				/* TODO: Write command substitude code here */

			case '$':
			/* TODO: Write variable, variable cut,
			 * ANSI-C style escape, command substitude,
			 * arithmetic expansion code here */
			case '>':
				if (ignore)
				{
					write_current();
					break;
				}
			case '<':
				/* TODO: Write input redirect and heredoc code
				 * here */
				write_current();
				break;
			case '#':
				if (cnt_buffer == cnt_first_nonIFS || (!strchr(" \t", buffer[cnt_buffer-1]))/* Is IFS */)
				{
					write_current();
					break;
				}
				if (cnt_argument_char == 0) /* Previously a blank reached */
				{
					cnt_argument_char = cnt_old_parameter;
					free(CUR_INFO->parameters[cnt_argument_element]);
					CUR_INFO->parameters[cnt_argument_element] = NULL;
					cnt_argument_element--;
				}
				write_char(0);
				return cnt_return;
			case '(':
			case ')':
			/* TODO: Write command sequence code here */
			case ';':
			/* TODO: Write muiltiple command process code here */
			default:
				write_current();
		}
	}
done:
	if (cnt_return > 0)
		write_char(0);
	free(buffer);
	return cnt_return;
}
