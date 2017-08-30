/*
 * =====================================================================================
 *       Filename:  builtin_command.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2013.11.01 15h31m28s
 *         Author:  wuyue (wy), vvuyve@gmail.com
 *        Company:  UESTC
 * =====================================================================================
 */
#include "pshell.h"

int builtin_command(char *command, char **parameters)
{
	extern struct passwd *pwd;
    if(strcmp(command,"exit")==0 || strcmp(command,"quit")==0)
        exit(0);
    else if(strcmp(command,"about") == 0)
    {
        printf("This is a simulation of shell (bash) in Linux.\n");
        return 1;
    }
    else if(strcmp(command,"cd")==0)
    {
        char *cd_path = NULL;

        if(parameters[1][0] == '~' || parameters[1] == NULL/* 'cd' */)
        {
            cd_path = malloc(strlen(pwd->pw_dir)+strlen(parameters[1]));
            //'~' makes length 1 more,but instead of '\0'
            if(cd_path == NULL)
            {
                printf("cd: malloc failed, please retry.\n");
            }
            strcpy(cd_path,pwd->pw_dir);
            strncpy(cd_path+strlen(pwd->pw_dir),parameters[1]+1,strlen(parameters[1]));
        }
        else
        {
            cd_path = malloc(strlen(parameters[1]+1));
            if(cd_path == NULL)
            {
                printf("cd: malloc failed, please retry.\n");
            }
            strcpy(cd_path,parameters[1]);
        }
        if(chdir(cd_path)!= 0)
            printf("cd: %s: %s\n", strerror(errno), cd_path);
        free(cd_path);
    }
    return 0;
}
