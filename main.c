/*
   main.c - main file of the psh

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

#include "pshell.h"
#include "backends/backend.h"
#include <setjmp.h>
#include <string.h>
#include <memory.h>

int status=0;
jmp_buf reset_point;
char *argv0;

void proc(void)
{
	int ParaNum;
	char prompt[MAX_PROMPT];
	char *buffer;
	struct parse_info info;
	prepare();
#ifndef NO_HISTORY
	using_history();
#endif
	if(setjmp(reset_point) == 1)
		printf("\n");
	while(1)
	{
		type_prompt(prompt);
		ParaNum = read_command(buffer, prompt, &info);
		if(ParaNum<0)
			continue;
		ParaNum--;/*count of units in buffer*/
		
		switch(run_builtin(&info))
		{
		case 1:
			break;
		case 2:
			status=1;
			break;
		default:
			do_run(&info);
			break;
		}
		free_parse_info(&info);
		memset(&info, 0, sizeof(info));
	}
}

int main(int argc, char **argv)
{
	argv0=strdup((strrchr(argv[0], '/')==NULL?argv[0]:strrchr(argv[0], '/')+1));

	if(argv0==NULL)
	{
		OUT2E("psh: strdup: No memory\n");
		exit(1);
	}
	proc();
	return 0;
}
