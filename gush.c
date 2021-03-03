#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/limits.h>
#include <fcntl.h>

/***************
**DECLARATIONS**
***************/

//running the shell
int gush( int mode );
char * getInput();
char **getArgs(char *input);
int executeCommand(char **args);

//error handling
int errorHandler();

//built-in commands
int gushExit(char **args);
int gushCd(char **args);
int gushPwd(char **args);
int gushPath(char **args);
int gushHistory(char **args);
int gushKill(char **args);

//helper functions
int redirectIn(char *argTok);
int redirectOut(char *argTok);

/*****************
**COMMAND ARRAYS**
*****************/

char *commandNames[] = { "exit", "cd", "pwd", "path", "history", "kill" };

int (*commandFuncs[])(char**) = { &gushExit, &gushCd, &gushPwd, &gushPath, &gushHistory, &gushKill };


int hCount = 0;	
char hArgs[20][64];

char *paths[64];

/******************
**INITIALIZATIONS**
******************/

int gush( int mode ) {
	int i = 0;
	while (i < 20) {
		strcpy(hArgs[i], "!");
		i++;
	}

	paths[0] = "/bin";

	int inHolder = dup(STDIN_FILENO);
	int outHolder = dup(STDOUT_FILENO);

	char *input = malloc(sizeof(char*));
	char *pInput = malloc(sizeof(char*));
	char *nextInput = malloc(sizeof(char*));
	char *inputCheck = malloc(sizeof(char*));
	char *hCopy;

	char DELIM[] = "&\n";

	pid_t pid;

	char **args = malloc(64*sizeof(char*));
	bool run = 1;

	while (run) {
		if (mode == 0) {
			printf("gush> ");
		}
		strcpy(input, getInput());
		
		pInput = strdup(input);

		nextInput = strtok(pInput, DELIM);
		strcpy(input, nextInput);
		while (nextInput != NULL) {
			nextInput = strtok(NULL, DELIM);
			pid = fork();
			if (pid == 0) {
				strcpy(input, nextInput);
				break;
			}	
		}

		inputCheck = strdup(input);
		if (inputCheck[0] == '!') {
			int num = inputCheck[1] - '0';
			if (num >= 1 && num <= 20) {
				strcpy(input, hArgs[num-1]);
			}
			else {
				errorHandler();	
			}
		}

		hCopy = strdup(input);	
		args = getArgs(input);	

		run = executeCommand(args);

		if (strcmp(args[0], "history") != 0) {
			strcpy(hArgs[hCount%20], hCopy);
			hCount++;
		}
		dup2(inHolder, STDIN_FILENO);
		dup2(outHolder, STDOUT_FILENO);

		if (pid == 0) {
			break;
		}
		else {
			wait(NULL);
		}
	}
	free(input);
	free(inputCheck);
	free(args);
	if (mode == 1) {
		exit(EXIT_SUCCESS);
	}
} //gush

char *getInput() {
	ssize_t inputLength = 0;
	char *input = NULL;
	
	getline(&input, &inputLength, stdin);

	if (feof(stdin)) {
		return "exit";
	}
	
	return input;	
} //getInput

char **getArgs(char *input) {
	char DELIM[] = " \t\r\n\v\f";
	int tokBuffer = 64;

	char **argToks = malloc(tokBuffer*sizeof(char*));
	char *argTok = malloc(sizeof(char*));
	char *nextTok = malloc(sizeof(char*));

	nextTok = strtok(input, DELIM);	
	if (nextTok != NULL) {
		strcpy(argTok, nextTok);
	}

	if (!argToks) {
		errorHandler();
	}

	int i = 0;
	while (argTok != NULL) {
		nextTok = strtok(NULL, DELIM);
		if (strcmp(argTok, "<") == 0) {
			redirectIn(nextTok);
			nextTok = strtok(NULL, DELIM);
		}
		else if (strcmp(argTok, ">") == 0) {
			redirectOut(nextTok);
			nextTok = strtok(NULL, DELIM);
		}
		else {
			argToks[i] = argTok;
		}
		argTok = nextTok;
		i++;
	}

	argToks[i] = NULL;

	return argToks;
} //getArgs

int executeCommand(char **args) {
	if (args[0] == NULL) {
		return errorHandler();
	}
	
	int i = 0;
	int commandsLength = sizeof(commandNames) / sizeof(char *);
	while (i < commandsLength) {
		if (strcmp(args[0], commandNames[i]) == 0) {
			return (*commandFuncs[i])(args);
		}
		i++;
	}

	int run = 1;
	pid_t pid, wpid;

	pid = fork();
	if (pid == 0) {
		int j = 0;
		char* testPath = malloc(sizeof(char*));
		if (access(args[0], F_OK) != 0) {
			while (paths[j] != NULL) {
				strcpy(testPath, paths[j]);
				strcat(testPath, "/");
				strcat(testPath, args[0]);
				if (access(testPath, F_OK) == 0) {
					strcpy(args[0], testPath);
					break;
				}
				j++;
			}
		}
		
		if (execve(args[0], args, NULL) == -1) {
			errorHandler();
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0) {
		errorHandler();
	}
	else {
		do {
			wpid = waitpid(pid, &run, WUNTRACED);
		} while (!WIFEXITED(run) && !WIFSIGNALED(run));
	}	

	return 1;
} //executeCommandi

int errorHandler() {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
	return 1;
} //errorHandler

/**BUILT-IN*COMMANDS**/

int gushExit(char **args) {
	if (args[1] != NULL) {
		return errorHandler();
	}
	return 0;
} //gushExit

int gushCd(char **args) {
	int argsLength = sizeof(args) / sizeof(args[0]);
	if (argsLength > 1) {
		return errorHandler();
	}

	if (args[1] != NULL) {
		chdir(args[1]);
	}	

	return 1;
} //gushCd

int gushPwd(char **args) {
	if (args[1] != NULL) {
		return errorHandler();
	}

	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	printf("%s\n", cwd);

	return 1;
} //gushPwd

int gushPath(char **args) {
	int i = 0;
	while (paths[0] != NULL) {
		paths[i] = NULL;
		i++;
	}

	char *copy = malloc(sizeof(char*));
	int j = 0;
	while (args[j] != NULL) {
		copy = strdup(args[j]);
		paths[j-1] = copy;
		j++;
	}

	return 1;
} //gushPath

int gushHistory(char **args) {
	if (args[1] != NULL) {
		return errorHandler();
	}

	if (strcmp(hArgs[hCount-1], "!") == 0) {
		return errorHandler();
	}

	int i = 0;
	int num = 1;
	int count = hCount;
	while (i < 20) {
		if (strcmp(hArgs[i], "!") != 0) {
			printf("%d %s\n", num, hArgs[i]);
			num++;
		}
		count++;
		i++;
		
	}
	return 1;
}

int gushKill(char **args) {
	if (args[2] != NULL) {
		return errorHandler();
	}
	if (args[1] == NULL) {
		return errorHandler();
	}

	char *killArg = args[0];
	strcat(killArg, " ");
	strcat(killArg, args[1]);

	system(killArg);

	return 1;
}

/**HELPER*FUNCTIONS**/

int redirectIn(char *argTok) {
	int redirectIn;
       	redirectIn = open(argTok, O_RDONLY);
	dup2(redirectIn, STDIN_FILENO);	
	return 1;
}

int redirectOut(char *argTok) {
	int redirectOut;
	redirectOut = open(argTok, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	dup2(redirectOut, STDOUT_FILENO);
	return 1;
}

/*******
**MAIN**
*******/

int main( int argc, char *argv[] ) {
	int mode = 0;
	
	if (argc == 1) {
		gush(mode);
	}
	else if (argc == 2) {
		mode = 1;
		char *file;
	       	file = argv[1];
		int inHolder = dup(STDIN_FILENO);
		int redirectIn = open(file, O_RDONLY);
		dup2(redirectIn, STDIN_FILENO);
		gush(mode);
		dup2(inHolder, STDIN_FILENO);
	}
	else {
		errorHandler();
	}
	
	return 1;
}
