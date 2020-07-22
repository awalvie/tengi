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


/* ------------------------------------------------------------------------ */
/* Built In Functions */
int tengi_cd(char **args);

char *builtin[] = {
	"cd",
};

int (*builtin_func[])(char **) = {
	&tengi_cd,
};

int total_builtins()
{
	return sizeof(builtin)/sizeof(char *);
}

int tengi_cd(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "cd needs at least one argument");
	} else {
		if (chdir(args[1]) != 0) {
			perror("chdir");
		}
	}
	return 1;
}

/* ------------------------------------------------------------------------ */

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
	int arg_pos = 0;
	int bufsize = ARG_BUF;
	char *token;
	char **args = malloc(bufsize * sizeof(char*));

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
			args = realloc(args, bufsize * sizeof(char*));
			if (!args) {
				fprintf(stderr, "Could not realloc memory for input");
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
		/* child */
		if (execvp(args[0], args) == -1) {
			perror("execvp");
			exit(EXIT_FAILURE);
		}
	} else if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else {
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

	for (i=0; i < total_builtins(); i++) {
		if (strcmp(args[0], builtin[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return execute(args);

}

int main(void)
{
	do {
		print_prompt();
		char *line = read_line();
		char **args = split_args(line);
		/* serach if first word is a builtin, if yes, call it, */
		/* otherwise execute the command with arguments */
		tengi_run(args);
		/* free things */
		free(line);
		free(args);
	} while (true);
	return 0;
}
