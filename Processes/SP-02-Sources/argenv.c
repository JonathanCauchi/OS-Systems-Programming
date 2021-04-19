#include <stdio.h>

extern char **environ;

int main(int argc, char **argv) {
	// Print out command line arguments
	for (int i = 0; i < argc; ++i)
		printf("argv[%d] = %s\n", i, argv[i]);

	// Print out environment strings
	for (int i = 0; environ[i] != NULL; ++i)
		printf("environ[%d] = %s\n", i, environ[i]);
}