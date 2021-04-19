#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

extern const char * const sys_siglist[];

static void sig_usr(int signo) {
	psignal(signo, "Using psignal");
	printf("Using strsignal: %s\n", strsignal(signo));
	printf("Using sys_siglist: %s\n", sys_siglist[signo]);
}

int main(int argc, char *argv[]) {
	if (signal(SIGUSR1, sig_usr) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR1!\n");
	if (signal(SIGUSR2, sig_usr) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR2!\n");

	for (;;)
		pause();
}
