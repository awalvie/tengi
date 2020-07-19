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

char *builtin_str[] = {
	"true",
	"false",
	"cat",
	"pwd",
	"echo",
};

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
		if (c == EOF || c == '\n')
			return buffer;
		else
			buffer[buf_pos] = c;

		buf_pos++;
		if (buf_pos >= READ_LINE_BUF) {
			bufsize += READ_LINE_BUF;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "Cannot realloc memory for input");
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
	char **args = malloc(ARG_BUF * sizeof(char*));
	char *token;
	int arg_pos = 0;

	if (!args) {
		fprintf(stderr, "Could not allocate buffer for args");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, TOKEN_DELIM);
	while (token != NULL) {
		args[arg_pos] = token;
		token = strtok(NULL, TOKEN_DELIM);

		if (arg_pos >= ARG_BUF) {
			args = realloc(args, ARG_BUF);
			if (!args) {
				fprintf(stderr, "Could not realloc memory for input");
				exit(EXIT_FAILURE);
			}
		}
	}

	return args;
}

int execute(char **args)
{
	pid_t pid, wait_id;
	pid = fork();
	int p_status;

	if (pid == 0) {
		if (execvp(args[0], args) == -1) {
			fprintf(stderr, "Could not call exec");
			exit(EXIT_FAILURE);
		}
	} else if (pid < 0) {
		fprintf(stderr, "Could not fork process");
		exit(EXIT_FAILURE);
	} else {
		do {
			wait_id = waitpid(pid, &p_status, WUNTRACED);
		} while (!WIFEXITED(p_status) && !WIFSIGNALED(p_status));
	}

	return 1;
}

int main(void)
{
	do {
		print_prompt();
		char *line = read_line();
		char **args = split_args(line);

		/* serach if first word is a builtin, if yes, call it, */
		/* otherwise execute the command with arguments */
		execute(args);


		free(line);
		free(args);
	} while (true);
	return 0;
}
