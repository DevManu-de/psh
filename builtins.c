/*
 * =====================================================================================
 *	   Filename:  builtin_command.c
 *	Description:  
 *		Version:  1.0
 *		Created:  2013.11.01 15h31m28s
 *		 Author:  wuyue (wy), vvuyve@gmail.com
 *		Company:  UESTC
 * =====================================================================================
 */
#include "pshell.h"
#include "builtins/builtin.h"
#define cmdis(cmd) (strcmp(command,cmd) == 0)

int run_builtin(char *command, char **parameters)
{
	extern struct passwd *pwd;
	if(cmdis("exit") || cmdis("quit"))
	{
		if(parameters[1]==NULL)
			exit(0);
		else
			exit(atoi(parameters[1]));
	}
	else if(cmdis("about"))
	{
		printf("psh is a not fully implemented shell in UNIX.\n");
		return 1;
	}
	else if(cmdis("cd")||cmdis("chdir"))
        return builtin_cd(command, parameters);
	else if(cmdis("echo"))
        return builtin_echo(command, parameters);
	else if(cmdis("exec"))
        return builtin_exec(command, parameters);
	else if(cmdis("export")||cmdis("alias"))
	{
		OUT2E("psh: %s: Not supported\n", command);
		return 1;
	}
	return 0;
}
