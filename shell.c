#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h> 

#define INPUT_STRING_SIZE 80

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

int cmd_quit(tok_t arg[]) {
	printf("Bye\n");
	exit(0);
	return 1;
}

int cmd_help(tok_t arg[]);
int cmd_cd(tok_t arg[]);

/* Command Lookup table */
typedef int cmd_fun_t (tok_t args[]); /* cmd functions take token array and return int */
typedef struct fun_desc {
	cmd_fun_t *fun;
	char *cmd;
	char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
		{cmd_help, "?", "show this help menu"},
		{cmd_quit, "quit", "quit the command shell"},
		{cmd_cd,"cd","change working directory"},
};
int cmd_cd(tok_t arg[]){
	if(chdir(arg[0])==-1)
		printf("  bash: cd: %s: No such file or directory\n",arg[0]);
	return 1;
}
int cmd_help(tok_t arg[]) {
	int i;
	for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
		printf("%s - %s\n",cmd_table[i].cmd, cmd_table[i].doc);
	}
	return 1;
}

int lookup(char cmd[]) {
	int i;
	for (i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
		if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
	}
	return -1;
}

void init_shell()
{
	/* Check if we are running interactively */
	shell_terminal = STDIN_FILENO;

	/** Note that we cannot take control of the terminal if the shell
      is not interactive */
	shell_is_interactive = isatty(shell_terminal);

	if(shell_is_interactive){

		/* force into foreground */
		while(tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp()))
			kill( - shell_pgid, SIGTTIN);

		shell_pgid = getpid();
		/* Put shell in its own process group */
		if(setpgid(shell_pgid, shell_pgid) < 0){
			perror("Couldn't put the shell in its own process group");
			exit(1);
		}

		/* Take control of the terminal */
		tcsetpgrp(shell_terminal, shell_pgid);
		tcgetattr(shell_terminal, &shell_tmodes);
	}
	/** YOUR CODE HERE */
}

/**
 * Add a process to our process list
 */
void add_process(process* p)
{
	/** YOUR CODE HERE */
}

/**
 * Creates a process given the inputString from stdin
 */
process* create_process(char* inputString)
{
	/** YOUR CODE HERE */
	return NULL;
}

char * concat(char * s1, char * s2){
	char * result;
	result = malloc(strlen(s1)+strlen(s2)+1);
	strcpy(result,s1);
	strcat(result,s2);
	return result;
}

void pathResolve(tok_t * t){
	char * path_env=getenv("PATH"),* path;
	tok_t * pathToks = getToks(path_env);
	int i;
	for(i=0;i<MAXTOKS && pathToks[i];i++){
		path = concat("/",t[0]);
		path = concat(pathToks[i],path);
		if(access(path,F_OK)!=-1){
			execve(path,t,NULL);
			//exit(0);
		}		
	}
	//part 2
	execv(*t,t);
	perror(*t);
	exit(0);
}

void redirect(tok_t *input,char * filename){ 
	int newfd; 

	if ((newfd = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
		perror(input);
		exit(1);
	}
	dup2(newfd, 1);

	pathResolve(input);
}

int shell (int argc, char *argv[]) {
	char *s = malloc(INPUT_STRING_SIZE+1);			/* user input string */
	tok_t *t;			/* tokens parsed from input */
	int lineNum = 0;
	int fundex = -1;
	pid_t pid = getpid();		/* get current processes PID */
	pid_t ppid = getppid();	/* get parents PID */
	pid_t cpid, tcpid, cpgid;
	char dir[100] ="";
	init_shell();

	printf("%s running as PID %d under %d\n",argv[0],pid,ppid);
	//getcwd(dir,100);
	lineNum=0;
	fprintf(stdout, "%d %s: ", lineNum,getcwd(dir,100));
	while ((s = freadln(stdin))){
		t = getToks(s); /* break the line into tokens */
		fundex = lookup(t[0]); /* Is first token a shell literal */
		if(fundex >= 0) cmd_table[fundex].fun(&t[1]);
		else {
			pid = fork();
			if(pid<0)
				perror("Fork process failed");
			if(pid==0){
				char *temp = strstr(s,">");
				int i=0;
				char * a=">", * b = "<";
				for(i=0;i<MAXTOKS && t[i];i++){
					if (strcmp( t[i], a) == 0){
						t[i]=NULL;
						printf("%s",t[i+1]);
						redirect(t,t[i+1]);	
					}
					if(strcmp( t[i], b) == 0){
						t[i]=NULL;
						printf("%s",t[i+1]);
						redirect(t,t[i+1]);
					}
				}
				pathResolve(t);


			}
			wait(NULL);
		}
		lineNum++;
		getcwd(dir,100);
		fprintf(stdout, "%d %s: ", lineNum,getcwd(dir,100));
	}
	return 0;
}
