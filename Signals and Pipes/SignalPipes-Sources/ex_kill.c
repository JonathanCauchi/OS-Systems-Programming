#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h>

extern const char * const sys_siglist[];

int is_number(char *s) {
	for (; *s; ++s) 
		if (!isdigit(*s))
			return false;

	return true;
}

int main(int argc, char *argv[]) 
{
	if (argc < 3) {
		fprintf(stderr, "Usage: skill signo pid-1 <pid-2 ... pid-n>\n");
		exit(EXIT_FAILURE);
	}

	if (!is_number(argv[1])) {
		fprintf(stderr, "Syntax error : signo expected integer\n");
		exit(EXIT_FAILURE);
	}

	// ASCII string to integer
	int signo = atoi(argv[1]);

	// Iterate through all pids
	for (int n = 2; argv[n] != NULL; n++) {
		if (is_number(argv[n])) {
			pid_t pid = atoi(argv[n]);
			
			printf("Signal [%s -> %d :: %d]\n", 
				sys_siglist[signo], pid, 
				kill(pid, signo));
		} else 
			printf("Ignoring pid [%s]\n", argv[n]);
	}

	exit(EXIT_SUCCESS);
}