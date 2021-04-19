#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

extern const char * const sys_siglist[];

static void sig_alrm(int signo) {
	printf("Received: %s\n", sys_siglist[signo]);
}

int main(int argc, char *argv[]) {
	if (signal(SIGALRM, sig_alrm) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGALRM!\n");

	alarm(3);
	pause();

	printf("SIGALRM expired!\n");
}
