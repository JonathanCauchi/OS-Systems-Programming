#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	printf("Parent process before fork()\n");

	pid_t pid = fork();

	// parent process
	if (pid > 0) {
		printf("This is the parent process! PID is [%d]\n", pid);
	// child process
	} else if (pid == 0) {
		printf("This is the child process! PID is [%d]\n", pid);
	// error
	} else {
		perror("fork() failed");
		exit(1);
	}
}
