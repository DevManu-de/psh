/*
 * =====================================================================================
 *	   Filename:  read_command.c
 *	Description:
 *		Version:  1.0
 *		Created:  2013.10.21 14h12min24s
 *		 Author:  wuyue (wy), vvuyve@gmail.com
 *		Company:  UESTC
 * =====================================================================================
 */
#include "pshell.h"

//return value: number of parameters
//0 represents only command without any parameters
//-1 represents wrong input
int read_command(char **command,char **parameters,char *prompt)
{
	memset(buffer, 0, sizeof(buffer));
	buffer = readline(prompt);
	if(feof(stdin))
	{
		printf("\n");
		exit(0);
	}
	/* Add history if buffer isn't blank */
	if(buffer && *buffer)
		add_history(buffer);

	buffer=preprocess_cmdline(buffer);

	if(buffer[0] == '\0')
		return -1;
	int count=split_buffer(command, parameters);
#ifdef DEBUG
	/*input analysis*/
	printf("input analysis:\n");
	printf("pathname:[%s]\ncommand:[%s]\nparameters:\n",*command,parameters[0]);
	int i;
	for(i=0; i<count-1; i++)
		printf("[%s]\n",parameters[i]);
#endif

	return count;
}
