#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static void sig_usr(int signo) {
	switch (signo) {
		case SIGUSR1:
			printf("Received SIGUSR1!\n");
			break;
		case SIGUSR2:
			printf("Received SIGUSR2!\n");
			break;
		default:
			printf("Received signal %d\n", signo);
	}
}

int main(int argc, char *argv[]) {
	if (signal(SIGUSR1, sig_usr) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR1!\n");
	if (signal(SIGUSR2, sig_usr) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR2!\n");

	for (;;)
		pause();
}
