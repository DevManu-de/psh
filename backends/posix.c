/*
   posix.c - backend of POSIX systems 

   Copyright 2013 wuyue.
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

#include "backend.h"
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

pid_t ChdPid, ChdPid2;
pid_t BPTable[MAXPIDTABLE]= {0};
int pipe_fd[2], in_fd, out_fd;
extern char *argv0;/*main.c*/

void sigchld_handler(int sig)
{
	pid_t pid;
	int i;
	for(i=0; i<MAXPIDTABLE; i++)
		if(BPTable[i] != 0) /*only handler the background processes*/
		{
			pid = waitpid(BPTable[i],NULL,WNOHANG);
			if(pid > 0)
			{
				printf("[%d] %d done\n", i+1, pid);
				BPTable[i] = 0; /*clear*/
			}
			else if(pid < 0)
			{
				if(errno != ECHILD)
					OUT2E("%s: waitpid error: %s", argv0, strerror(errno));
			}
			/*else:do nothing.*/
			/*Not background processses has their waitpid() in wshell.*/
		}
	return;
}

void sigintabrt_hadler(int sig)
{
	status=sig;
	return;
}

int prepare(void)
{
	int ret=0;
	if(signal(SIGCHLD,sigchld_handler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)),ret++;

	if(signal(SIGINT,sigintabrt_hadler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)),ret++;

	if(signal(SIGQUIT,sigintabrt_hadler) == SIG_ERR)
		OUT2E("%s: signal error: %s", argv0, strerror(errno)),ret++;
	return ret;
}

char *gethd(void)
{
	struct passwd *pwd=getpwuid(getuid());
	if(pwd==NULL)
		return NULL;
	return pwd->pw_dir;
}

char *getun(void)
{
	struct passwd *pwd=getpwuid(getuid());
	if(pwd==NULL)
		return NULL;
	return pwd->pw_name;
}

char *pshgetcwd(void)
{
	char *cwd=malloc(4097);
	getcwd(cwd, 4097);
	return cwd;
}

int pshgethostname(char *hstnme, size_t len)
{
	return gethostname(hstnme, len);
}

int do_run(char *command, char **parameters, struct parse_info info)
{
	if(info.flag & IS_PIPED) /*command is not null*/
	{
		if(pipe(pipe_fd)<0)
		{
			OUT2E("%s: pipe failed: %s\n", argv0, strerror(errno));
			exit(0);
		}
	}
	if((ChdPid = fork())!=0) /*shell*/
	{
		if(info.flag & IS_PIPED)
		{
			if((ChdPid2=fork()) == 0) /*command*/
			{
				close(pipe_fd[1]);
				close(fileno(stdin));
				dup2(pipe_fd[0], fileno(stdin));
				close(pipe_fd[0]);
				execvp(info.command,info.parameters);
			}
			else
			{
				close(pipe_fd[0]);
				close(pipe_fd[1]);
				waitpid(ChdPid2,&status,0); /*wait command*/
			}
		}

		if(info.flag & BACKGROUND)
		{
			int i;
			for(i=0; i<MAXPIDTABLE; i++)
				if(BPTable[i]==0)
				{
					BPTable[i] = ChdPid; /*register a background process*/
					break;
				}

			printf("[%d] %u\n", i+1, ChdPid);
			if(i==MAXPIDTABLE)
				OUT2E("%s: Too much background processes\n", argv0);
			usleep(5000);
		}
		else
		{
			waitpid(ChdPid,&status,0);/*wait command1*/
		}
	}
	else /*command1*/
	{

		if(info.flag & IS_PIPED) /*command is not null*/
		{
			if(!(info.flag & OUT_REDIRECT) && !(info.flag & OUT_REDIRECT_APPEND)) /* ONLY PIPED*/
			{
				close(pipe_fd[0]);
				close(fileno(stdout));
				dup2(pipe_fd[1], fileno(stdout));
				close(pipe_fd[1]);
			}
			else /*OUT_REDIRECT and PIPED*/
			{
				close(pipe_fd[0]);
				close(pipe_fd[1]);/*send a EOF to command*/
				if(info.flag & OUT_REDIRECT)
					out_fd = open(info.out_file, O_WRONLY|O_CREAT|O_TRUNC, 0666);
				else
					out_fd = open(info.out_file, O_WRONLY|O_APPEND|O_TRUNC, 0666);
				close(fileno(stdout));
				dup2(out_fd, fileno(stdout));
				close(out_fd);
			}
		}
		else
		{
			if(info.flag & OUT_REDIRECT) /* OUT_REDIRECT WITHOUT PIPE*/
			{
				out_fd = open(info.out_file, O_WRONLY|O_CREAT|O_TRUNC, 0666);
				close(fileno(stdout));
				dup2(out_fd, fileno(stdout));
				close(out_fd);
			}
			if(info.flag & OUT_REDIRECT_APPEND) /* OUT_REDIRECT_APPEND WITHOUT PIPE*/
			{
				out_fd = open(info.out_file, O_WRONLY|O_CREAT|O_APPEND, 0666);
				close(fileno(stdout));
				dup2(out_fd, fileno(stdout));
				close(out_fd);
			}
		}

		if(info.flag & IN_REDIRECT)
		{
			in_fd = open(info.in_file, O_CREAT |O_RDONLY, 0666);
			close(fileno(stdin));
			dup2(in_fd, fileno(stdin));
			close(in_fd);
		}
		if(execvp(command,parameters)==-1)
		{
			if(errno == ENOENT)
				OUT2E("%s: %s: command not found\n", argv0, command);
			else
				OUT2E("%s: %s: %s\n", argv0, command, strerror(errno));
			/* Exit the failed command child process */
			_Exit(1);
		}
	}
	return 0;
}
