#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	printf("Running execl()...\n");
	
	// call execl followed by argument list and NULL
	if (execl("/bin/ls", "ls", NULL)) {
		perror("execl failed:");
		exit(1);
	}

	// never executes
	printf("This message will never be shown.\n");
}