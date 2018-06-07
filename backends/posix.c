/*
   posix.c - backend of POSIX systems

   Copyright 2013 wuyve.
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

#include <fcntl.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "backend.h"

pid_t ChdPid, ChdPid2;
pid_t BPTable[MAXPIDTABLE] = {0};
int pipe_fd[2], in_fd, out_fd;
extern char *argv0;	 /*main.c*/
extern jmp_buf reset_point; /*main.c*/

void sigchld_handler(__attribute__((unused)) int sig)
{
	pid_t pid;
	int i;
	for (i = 0; i < MAXPIDTABLE; i++)
		if (BPTable[i] != 0) /*only handler the background processes*/
		{
			pid = waitpid(BPTable[i], NULL, WNOHANG);
			if (pid > 0)
			{
				printf("[%d] %d done\n", i + 1, pid);
				BPTable[i] = 0; /*clear*/
			}
			else if (pid < 0)
			{
				if (errno != ECHILD)
					OUT2E("%s: waitpid error: %s", argv0, strerror(errno));
			}
			/*else:do nothing.*/
			/*Not background processses has their waitpid() in
			 * wshell.*/
		}
	return;
}

void sigintabrt_hadler(int sig)
{
	last_command_status = sig;
	longjmp(reset_point, 1);
}

int prepare(void)
{
	int ret = 0;
	if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)), ret++;

	if (signal(SIGINT, sigintabrt_hadler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)), ret++;

	if (signal(SIGQUIT, sigintabrt_hadler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)), ret++;
	return ret;
}

char *gethd(void)
{
	struct passwd *pwd = getpwuid(getuid());
	if (pwd == NULL)
		return NULL;
	return pwd->pw_dir;
}

char *gethdnam(char *username)
{
	struct passwd *pwd = getpwnam(username);
	if (pwd == NULL)
		return NULL;
	return pwd->pw_dir;
}

char *getun(void)
{
	struct passwd *pwd = getpwuid(getuid());
	if (pwd == NULL)
		return NULL;
	return pwd->pw_name;
}

char *pshgetcwd(void)
{
	char *cwd = malloc(4097);
	if (cwd == NULL)
		return NULL;
	getcwd(cwd, 4097);
	return cwd;
}

char *pshstrdup(char *str) { return strdup(str); }

int pshgethostname(char *hstnme, size_t len) { return gethostname(hstnme, len); }

int pshgetuid(void) { return geteuid(); }

int pshchdir(char *dir) { return chdir(dir); }

int pshsetenv(const char *name, const char *value, int overwrite) { return setenv(name, value, overwrite); }

static int redir_spawnve(struct redirect *arginfo, char *cmd, char **argv, char **env)
{
	pid_t pid;
	struct redirect *info = arginfo;
	if ((pid = fork()) == 0)
	{
		while (info)
		{
			switch (info->type)
			{
				case FD2FD:
					dup2(info->in.fd, info->out.fd);
					close(info->in.fd);
					break;
				case OUT_REDIR:
					dup2(open(info->out.file, O_WRONLY | O_CREAT | O_TRUNC, 0644), info->in.fd);
					break;
				case OUT_APPN:
					dup2(open(info->out.file, O_WRONLY | O_CREAT | O_APPEND, 0644), info->in.fd);
					break;
				case IN_REDIR:
					dup2(open(info->in.file, O_RDONLY | O_CREAT, 0644), info->out.fd);
					break;
				case CLOSEFD:
					close(info->in.fd);
					break;
				case OPENFN:
					dup2(open(info->in.file, O_RDWR | O_CREAT, 0644), info->out.fd);
					break;
			}
			info = info->next;
		}
		execve(cmd, argv, env);
	}

	return pid;
}

int do_run(struct command *arginfo)
{
	struct command *info = arginfo;
	int i = 0;
	printf("--**--\nstub!\nflags won't be read\n");
	printf("info position: %p\n", arginfo);
	while (++i)
	{
		int j;
		printf("part %d:\n"
		       "command: %s\n"
		       "params:\n",
		       i, info->parameters[0]);
		for (j = 0; info->parameters[j]; ++j)
			printf("%s\n", info->parameters[j]);
		printf("flag: %d\n", info->flag);
		printf("redir type: %d\n", (info->rlist != NULL) ? info->rlist->type : 0);

		if (info->next == NULL)
			break;
		info = info->next;
	}
	printf("--*END*--\n");
	if ((ChdPid = fork()) != 0) /*shell*/
	{
		waitpid(ChdPid, &last_command_status, 0); /*wait command1*/
	}
	else /*command1*/
	{
		if (execvp(info->parameters[0], (char **)info->parameters) == -1)
		{
			if (errno == ENOENT && !strchr(info->parameters[0], '/'))
			{
				OUT2E("%s: %s: command not found\n", argv0, info->parameters[0]);
				_Exit(127);
			}
			else
			{
				OUT2E("%s: %s: %s\n", argv0, info->parameters[0], strerror(errno));
				/* Exit the failed command child process */
				_Exit(126);
			}
		}
	}
	return 0;
}

/* * *
 * Platform dependent builtins part
 *  * *
 */
#include "builtins/builtin.h"

/* Builtin exec */
int builtin_exec(ARGS)
{
	if (b_parameters[1] == NULL)
		return 1; /* Do nothing */
	if (execv(b_parameters[1], &b_parameters[1]) == -1)
		OUT2E("exec: %s: %s\n", b_parameters[1], strerror(errno));
	return 2;
}
