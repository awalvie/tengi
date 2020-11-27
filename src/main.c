#include <sys/wait.h>
#include <wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#define TOKEN_DELIM " \n\t\r"
#define READ_LINE_BUF 1024
#define ARG_BUF 64

extern char **environ;

int in, out, input, output, append, dont_wait;
char *inputFile, *outputFile;
FILE *fp;

/* ------------------------------------------------------------------------ */
/* Built In Functions */
int tengi_cd(char **args);
int tengi_exit(char **args);
int tengi_env();

char *builtin[] = {
	"cd",
	"exit",
	"env",

};

int (*builtin_func[])(char **) = {
	&tengi_cd,
	&tengi_exit,
	&tengi_env,
};

int total_builtins()
{
	return sizeof(builtin) / sizeof(char *);
}

int tengi_cd(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "cd needs at least one argument\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("chdir");
		}
	}
	return 1;
}

int tengi_exit(char **args)
{
	printf("\nExiting tengi, Bye!\n");
	return 0;
}

int tengi_env()
{
	char **env = environ;

	if (output == 1) {
		fp = fopen(outputFile, "w");
	} else if (append == 1) {
		fp = fopen(outputFile, "a");
	}

	if (output == 1 || append == 1) {
		while (*env) {
			fprintf(fp, "%s\n", *env++);
		}
		fclose(fp);
	}

	else {
		while (*env) {
			printf("%s\n", *env++);
		}
	}
	return 1;
}

/* ------------------------------------------------------------------------ */

void checkIO(char **args)
{
	input = 0;
	output = 0;
	append = 0;

	int i = 0;

	while (args[i] != NULL) {
		if (!strcmp(args[i], "<")) {
			strcpy(args[i], "\0");
			inputFile = args[i + 1];
			input = 1;
		} else if (!strcmp(args[i], ">")) {
			outputFile = args[i + 1];
			args[i] = NULL;
			output = 1;
			break;
		} else if (!strcmp(args[i], ">>")) {
			outputFile = args[i + 1];
			args[i] = NULL;
			append = 1;
			break;
		}
		i++;
	}
}

int checkBackground(char **args)
{
	int i = 0;
	int dont_wait = 0;
	while (args[i] != NULL) {
		if (!strcmp(args[i], "&")) {
			dont_wait = 1;
			args[i] = NULL;
		}
		i++;
	}
	return dont_wait;
}

void print_prompt()
{
	printf("(tengi)$ ");
}

/* read line from stdin */
char *read_line(void)
{
	int bufsize = READ_LINE_BUF;
	int buf_pos = 0;
	int c;
	char *buffer = malloc(sizeof(char) * bufsize);

	if (!buffer) {
		fprintf(stderr, "Cannot allocate memory for line!");
		exit(EXIT_FAILURE);
	}

	while (1) {
		c = getchar();
		if (c == EOF || c == '\n') {
			buffer[buf_pos] = '\0';
			return buffer;
		} else {
			buffer[buf_pos] = c;
		}

		buf_pos++;
		if (buf_pos >= bufsize) {
			bufsize += READ_LINE_BUF;
			buffer = realloc(buffer, bufsize * sizeof(char));
			if (!buffer) {
				fprintf(stderr,
					"Cannot realloc memory for input");
				exit(EXIT_FAILURE);
			}
		}
	}
}

/* split argument on space */
/* TODO: currently "hello world" is not processed as a single
argument, it's split into "hello and world", provision it to
handle quotes and escape sequences*/
char **split_args(char *line)
{
	int arg_pos = 0;
	int bufsize = ARG_BUF;
	char *token;
	char **args = malloc(bufsize * sizeof(char *));

	if (!args) {
		fprintf(stderr, "Could not allocate buffer for args");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, TOKEN_DELIM);
	while (token != NULL) {
		args[arg_pos] = token;
		arg_pos++;
		if (arg_pos >= bufsize) {
			bufsize += ARG_BUF;
			args = realloc(args, bufsize * sizeof(char *));
			if (!args) {
				fprintf(stderr,
					"Could not realloc memory for input");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, TOKEN_DELIM);
	}
	args[arg_pos] = NULL;
	return args;
}

int execute(char **args)
{
	pid_t pid, wait_id;
	int p_status;

	pid = fork();
	/* read the man page for waitpid if having trouble understanding */
	if (pid == 0) {
		if (input == 1) {
			if (!access(inputFile, R_OK)) {
				freopen(inputFile, "r", stdin);
			}
		}
		if (output == 1)
			freopen(outputFile, "w", stdout);
		else if (append == 1)
			freopen(outputFile, "a+", stdout);

		/* child */
		if (execvp(args[0], args) == -1) {
			perror("execvp");
			exit(EXIT_FAILURE);
		}
	} else if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (!dont_wait) {
		/* parent */
		do {
			wait_id = waitpid(pid, &p_status, WUNTRACED);
		} while (!WIFEXITED(p_status) && !WIFSIGNALED(p_status));
	}
	return 1;
}

int tengi_run(char **args)
{
	int i;

	if (args[0] == NULL) {
		return 1;
	}

	checkIO(args);
	dont_wait = checkBackground(args);

	for (i = 0; i < total_builtins(); i++) {
		if (strcmp(args[0], builtin[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return execute(args);
}

void loop(void)
{
	int status = 0;
	char *line;
	char **args;

	do {
		print_prompt();
		line = read_line();
		args = split_args(line);
		/* serach if first word is a builtin, if yes, call it, */
		/* otherwise execute the command with arguments */
		status = tengi_run(args);
		/* free things */
		free(line);
		free(args);
	} while (status);
}

int main(void)
{
	loop();

	return EXIT_SUCCESS;
}
