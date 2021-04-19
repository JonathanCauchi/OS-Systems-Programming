#include <stdio.h>
#include <stdlib.h>

static void exit_handler(void) {
	printf("exit_handler() called from exit()\n");
}

int main(int argc, char **argv) {
	if (atexit(exit_handler))
		perror("Cannot register exit handler:");

	printf("Hello, world!\n");
}