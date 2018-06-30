#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include<string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"   /*include declarations for parse-related structs*/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT , JOBS , KILL ,HELP , HISTORY , CD , FG , BG, SETENV};

char *hist_arr[100000];
int hist_size = 100000;
char *background_jobs[100];

pid_t jobstatus[1000] ;

char dir_stack[1000][1000];
int stack_ptr = 0,empty = 0 , changed = 0;

int bgrunning = 0;
int jobptr = 0;
int flag = 0;
int print_flag = 0;
int got = 0 ;
static int rear = -1;
static int in_dex = 0;
  
#define buf_size 200 
char buf[buf_size]; 

HIST_ENTRY ** ht;

char * buildPrompt()
{ 
  return  "$ ";
}
 
int isBuiltInCommand(char * cmd)
{

	if ( strncmp(cmd, "exit", strlen("exit")) == 0)
	{
		return EXIT;
	}
	else if(strncmp(cmd, "jobs", strlen("jobs")) == 0)
	{
		return JOBS;
	}
	else if(strncmp(cmd, "kill", strlen("kill")) == 0)
	{
		return KILL;
	}
	
	else if(strncmp(cmd, "help", strlen("help")) == 0)
	{
		return HELP;
	}
	
	else if(strncmp(cmd, "history", strlen("history")) == 0)
	{
		return HISTORY;
	}
	else if(strncmp(cmd, "cd", strlen("cd")) == 0)
	{
		return CD;
	}
	else if(strncmp(cmd, "fg", strlen("fg")) == 0)
	{
		return FG;
	}
	else if(strncmp(cmd, "bg", strlen("bg")) == 0)
	{
		return BG;
	}
	else if(strncmp(cmd, "setenv", strlen("setenv")) == 0)
	{
		return SETENV;
	}
	return NO_SUCH_BUILTIN;
}


int main (int argc, char **argv)
{
	char * cmdLine;
	parseInfo *info; /*info stores all the information returned by parser.*/
	struct commandType *com; /*com stores command name and Arg list for one command.*/

	#ifdef UNIX
		fprintf(stdout, "This is the UNIX/LINUX version\n");
	#endif

	#ifdef WINDOWS
		fprintf(stdout, "This is the WINDOWS version\n");
	#endif
	
	
	
	while(1)
	{
		int val = 0; 
		char cmd[1024];
		
		int hist_done = 0;
		char delims[] = ">,<,|";
		
		char result[100] = "";  /* Original size was too small */
		char* token;
		
		char cmd1[1024];
		
		char strr[100];
		
		char *value;
		
		int jk = 0;
		
		ht = history_list();
		
		
		
		#ifdef UNIX
		if(flag == 0) /*This is check if this is a new command and not the command to be executed by !x or !-x*/
		{
		
			getcwd(cmd1, 1024);
			getcwd(cmd, 1024);  
			strcat(cmd1,"$ ");
			cmdLine = readline(cmd1);
			print_flag = 0;
		}
		else /*This is running the command at x or -x in the history section as previously !-x or !x was entered*/
		{
			
			cmdLine = ht[got]->line;
			flag = 0;
			print_flag = 1;
		}
		if (cmdLine == NULL) 
		{
			fprintf(stderr, "Unable to read command\n");
			continue;
    		}
		#endif
		
		using_history();
		
		/*calls the parser*/
		info = parse(cmdLine);
	
		if (info == NULL)
		{
			free(cmdLine);
			continue;
		}
		/*prints the info struct*/
		if(print_flag == 0)
		{
			print_info(info);
			printf("---------------------------------------------------------------------------------------------------\n");
		}	
		/*com contains the info. of the command before the first "|"*/
		
		
		com=&info->CommArray[0];
		
		/*com->command tells the command name of com*/
		if ((com == NULL)  || (com->command == NULL)) 
		{
			free_info(info);
			free(cmdLine);
			continue;
		}
		
		if (isBuiltInCommand(com->command) == EXIT)
		{
			if(jobptr > 0) /*Checks if there are background processes runnning and notifies the user to end it*/
			{
				printf("There are background processes running. So cant exit. First Kill them\n");
			}
			else/*Exits the shell*/
			{
				
				exit(-1);
			}
	    }
		
		else if(isBuiltInCommand(com->command) == HISTORY)/*This section stores the command in the history*/
		{
			int val = 0;
			
			add_history(cmdLine);
			
			if (com->VarList[1] == NULL)
			{
				ht = history_list();
			
				if(ht)
				{
					for(val=0; val < history_length ; val++)
						printf("%s\n",ht[val]->line);			
				}
				
			}
			else if(strncmp(com->VarList[1], "-s", strlen("-s")) == 0)
			{
				stifle_history(atoi(com->VarList[2]));
			}
			else
			{
				ht = history_list();
			
				if(ht)
				{
					for(val = (history_length-atoi(com->VarList[1])) ; val < history_length ; val++)
						printf("%s\n",ht[val]->line);			
				}
			}
			hist_done = 1;
		}
		else if(strncmp(&(com->VarList[0][0]), "!", strlen("!")) != 0)/*add the command in the history list*/
		{
			
			add_history(cmdLine);
		}
		
		if(isBuiltInCommand(com->command) == FG)
		{
			int stat;
			if(jobptr > 0)
			{
				waitpid(jobstatus[jobptr-1], &stat,0);
				printf("process resumed %d\n",jobstatus[jobptr-1]);
			}
			else
			{
				printf("No backgrocess running\n");
			}
		
		}
		else if(isBuiltInCommand(com->command) == BG)
		{
			if(bgrunning == (jobptr-1))/*goes here if all the processes have already been started */
					printf("[%d] %s already running\n",bgrunning+1,background_jobs[bgrunning]);
			else
			{
				int vi = (jobptr-bgrunning-1), statuss;
				pid_t ppgid;
				int as = waitpid(jobstatus[vi],&statuss,WNOHANG);
				ppgid = getsid (jobstatus[vi]);
				if(as !=0 & as != jobstatus[vi])
				{
					if (kill (ppgid, SIGCONT) < 0)/*gives the job signal to resume running in background and checks if successfully it has done it*/
      						perror ("kill (SIGCONT)");
					else
					{
						printf("[%d] running\t\t%s \n",vi,background_jobs[vi]);
						bgrunning += 1;
					}
				}	
			}
		}
		else if(isBuiltInCommand(com->command) == CD)/*IT changes the directory of the prompt*/
		{
			
			if(com->VarNum < 2) /* Takes it to the home directory*/
			{
				chdir("/home");
				
			}
			else
			{
				chdir(com->VarList[1]);
			}
			getcwd(cmd1, 1024); 
			
			if((strncmp(cmd1,cmd, strlen(cmd)) == 0) && (strncmp(cmd,cmd1, strlen(cmd1)) == 0))/*If the directory doesnot then displays an error*/
				printf("No such file or directory\n");
			
		}
		else if(isBuiltInCommand(com->command) == HELP)/*prints all these on the prompt*/
		{
			printf("The Following are the built-in commands : \n1. jobs \n2. cd \n3. history\n4. kill\n5. exit\n6. help\n");
			printf("\nThe jobs command prints the total number of background processes running\n");
			printf("Syntax : jobs\n\n");
			printf("\nThe cd command changes the working directory of the shell\n");
			printf("Syntax : cd <absolute or relative path> \n\n");
			printf("\nThe history displays the previous commands that were entered in the shell\n");
			printf("Syntax : history\n\n");
			printf("\nThe kill command kills that background processes that are already running in the shell\n");
			printf("Syntax : kill %%num i.e., the %%num kills the position num process among all the background process\n");
			printf("\nThe exit command exits the shell\n");
			printf("Syntax : exit\n\n");
			printf("\nThe help command gives info about all the built-in commands of the shell\n");
			printf("Syntax : help\n");	
		}
		else if(isBuiltInCommand(com->command) == SETENV)/*does the setenv command for editing the environment variables*/
		{
			setenv(com->VarList[1], com->VarList[2], 0);
		}
		else if(strncmp(com->VarList[0], "dirs", strlen("dirs")) == 0)/*prints the directories stored in the stack*/
		{
			if(empty == 0)
			{
				if(com->VarList[2] == NULL)
					printf("%s\n",cmd);
				else
					printf("0 %s\n",cmd);
			}
			else
			{
				
				if(com->VarList[2] == NULL)
				{
					printf("%s\n",cmd);
					for(jk=0 ; jk < stack_ptr ; jk++)
						printf("%s  ",dir_stack[jk]);
					printf("\n");
				}
				else
				{
					/*printf("hi %s\n",dir_stack[0]);*/
					printf("0 %s\n",cmd);
					for(jk=0 ; jk < stack_ptr ; jk++)
						printf("%d %s\n",jk+1,dir_stack[jk]);
				}
			}
		}
		else if(strncmp(com->VarList[0], "pushd", strlen("pushd")) == 0)/*executes the pushd command*/
		{
			
			if(stack_ptr == 0 & com->VarList[1] == NULL)
			{
				printf("bash: pushd: no other directory\n");
			}
			else
			{

				empty += 1;
				strcpy( dir_stack[stack_ptr] , cmd);
				printf("%s",cmd);
				for(jk=0 ; jk < stack_ptr ; jk++)
					printf("%s  ",dir_stack[jk]);
				printf("\n");
				stack_ptr += 1;
				chdir(com->VarList[1]);
				getcwd(cmd,1024);
			}
		}
		else if(strncmp(com->VarList[0], "popd", strlen("popd")) == 0)/*executes the popd command*/
		{
			if(empty == 0) 
			{
				printf("bash: popd: no directory found");
			}
			else
			{
				for(jk=1 ; jk < stack_ptr ; jk++)
					printf("%s  ",dir_stack[jk]);
				printf("\n");
				chdir(dir_stack[stack_ptr-1]);
				stack_ptr -= 1;
				empty -= 1;
			}
		}
		else if(info->boolBackground)/*Keeps track of backgrounds and executes it*/
  		{
  			pid_t pid;
  			pid = fork();
			
			if(0 == pid)/*Chils process*/
			{
				
				execvp(com->VarList[0],com->VarList);/*executes*/

			}
			else
			{
				
				background_jobs[jobptr] = (char *)malloc(100*sizeof(char)); /*Stores in array for getting all the jobs*/
				strcpy(background_jobs[jobptr],cmdLine);
				jobstatus[jobptr] = pid;	
				jobptr += 1;
			}
  		}
	  		
		
	  	else if(strncmp(&(com->VarList[0][0]), "!", strlen("!")) == 0)/*Executing the particular entry from the history list*/
	  	{
	  		if(history_length > 0)
	  		{
		  		char * pch;
	  			int val=0 , pin = 0;
	  			int childstatus;
	  			pid_t pid;
	  			pch = strtok (com->VarList[0],"!");
		  		val = atoi(pch);
		  		if(val < 0)
		  		{
		  			val = (val*-1) - 1;
		  			pin = history_length - 1 - val;
		  		
		  			got  = pin;
	  				flag = 1;
		  			
		  		}
		  		else
		  		{
		  			val = val-1;
	  				got = val;
	  				flag = 1;
		  		}
		  		
			}
			else
			{
				printf("No history Commands found\n");
			}
	  	}
	  	else if(isBuiltInCommand(com->command) == JOBS)/*prints all the background processes that are running*/
	  	{
	  		int val = 0,statuss;
	  		char *pch;
	  		
	  		if(jobptr==0)
	  			printf("No background jobs\n");
	  		else
	  		{
	  			
		  		for(val = 0; val < jobptr ; val++)
		  		{
		  			int as = waitpid(jobstatus[val],&statuss,WNOHANG);
		  			pch = strtok (background_jobs[val]," ");
		  			if(as == 0)
		  				printf("[%d] -Running\t\t%s\n",val+1,pch);
		  			else if(as == jobstatus[val])
		  				printf("[%d] -Terminated\t\t%s\n",val+1,pch);
		  			else
		  				printf("[%d] -Stopped\t\t%s\n",val+1,pch);
		  		}
		  	}
	  	}
	  	else if(isBuiltInCommand(com->VarList[0]) == KILL)/*Kill the jobs at num given by kill %num*/
	  	{
	  		
	  		char *pch;
	  		int ii = 0;
	  		pch = strtok (com->VarList[1],"%");
	  		val = atoi(pch) - 1;
	  		if(jobptr == 0)
	  			printf("bash: kill %%%d: no such job\n",val+1);
	  		else
	  		{
	  			kill(jobstatus[val],SIGKILL);
	  			printf("[%d] -Killed\t\t %s\n",val+1,background_jobs[val]);
			}	
	  		if(val == jobptr-1)
	  		{
	  			jobptr = jobptr - 1;
	  		}
	  		else
	  		{
	  			for(ii = val ; ii < jobptr ; ii++)
	  			{
	  				background_jobs[ii] = background_jobs[ii+1]; 
	  			}
	  			jobptr = jobptr - 1;
	  			
	  		}
	  	}
	  	else if(hist_done == 0) /*comes here are executes if it has I/O redirection or fg, bg process or pipe command*/
	  	{	
	  		int j = 0;
			int stat;
	  		if(info->pipeNum > 0)/*executes the piped commands*/
			{	
				int num_pipe = 0;
				
				int status;
				pid_t pid;
				
				if(fork()==0) 
				{ 
					 /* child */
						for(num_pipe = 0; num_pipe < info->pipeNum ; num_pipe++)
						{
							int mypipe[2];
							pipe(mypipe);
							
							if(fork()==0)
							{
												
									dup2(mypipe[1], 1);	
									execvp(info->CommArray[num_pipe].VarList[0],info->CommArray[num_pipe].VarList);
									printf("Command not found\n");
									perror(com->command);
							}							
						
							dup2(mypipe[0], 0);
							close(mypipe[1]);
						}
						execvp(info->CommArray[info->pipeNum].VarList[0],info->CommArray[info->pipeNum].VarList);
						printf("Command not found later\n");
						perror(com->command);
				}
				sleep(1);
	  		}	  		
	  		
	  		else if(info->boolOutfile & info->boolInfile)/*Input and output file redirection*/
	  		{
	  				int status,i=0;
	  				pid_t pid;
					pid = fork();
					
					if(0 == pid)
					{
						FILE *infile, *outfile;
						infile = fopen(info->inFile, "r");
						outfile = fopen(info->outFile, "w");
    						dup2(fileno(outfile), 1);
						dup2(fileno(infile), 0);
						execvp(com->command,com->VarList);
						printf("Command Not Found : couldnot perferm boolin and out\n");
						exit(1);					
					}
					else
					{
					
						wait(&status);
								
					}
	  		}
	  		else if(info->boolOutfile)/*output file redirection*/
	  		{	
					int status,i=0;
					char *s;
					
					FILE *infile ,*outfile;
					pid_t pid;
					
					pid = fork();
					
					if(0 == pid)
					{
						if(strncmp(&(com->VarList[0][0]), ".", strlen(".")) == 0)
						{
							outfile = fopen(info->outFile, "w");	
							dup2(fileno(outfile), 1);
							execvp(com->command,com->VarList);
							printf("Command not found\n");
						}
						else
						{
							infile = fopen(info->inFile, "r");
							outfile = fopen(info->outFile, "w");
							dup2(fileno(infile), 0);   
							dup2(fileno(outfile), 1);  
						}
						exit(1);	
					}
					else
					{
					
						wait(&status);
								
					}
	  			
	  		}
	  		else if(info->boolInfile)/*input file redirection*/
	  		{
	  				int status,i=0;
					FILE *infile;
					pid_t pid;
					
					pid = fork();
					
					if(0 == pid)
					{
						FILE *infile, *outfile;
						infile = fopen(info->inFile, "r");
						dup2(fileno(infile), 0);
						execvp(com->command,com->VarList);
						printf("Command Not Found\n");
						exit(1);					
					}
					else
					{
					
						wait(&status);
								
					}
	  		}
	  		
	  		else /*Any command which is given in relative or absolute path will be executed from here*/
	  		{
			  	int childstatus;
				pid_t pid;
				pid = fork();	
				if(0 == pid )
				{
					execvp( com->command,com->VarList);
					printf("Command Not Found\n");
					exit(1);
				}
				else
				{
					wait(&childstatus);
				}
		  	}
	  	}
    		

    		free_info(info);
    		free(cmdLine);
    	}
    	/* while(1) */
}
