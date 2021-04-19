#include <stdio.h>
#include <signal.h>
#include <unistd.h>

extern const char * const sys_siglist[];

sigset_t set;

void signal_handler(int signo) {
	sigset_t oldset;

	sigprocmask(SIG_BLOCK, &set, &oldset);
	
	printf("Signals blocked while handling: %s; sleeping for 10 s...\n", sys_siglist[signo]);
	sleep(10);
	printf("Ready; unblocking signals...\n");

	sigprocmask(SIG_UNBLOCK, &oldset, NULL);
}

int main(int argc, char *argv[]) {

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);

	if (signal(SIGUSR1, signal_handler) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR1!\n");
	if (signal(SIGUSR2, signal_handler) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGUSR2!\n");
	if (signal(SIGALRM, signal_handler) == SIG_ERR)
		fprintf(stderr, "Can't catch SIGALRM!\n");

	printf("Calling alarm(5)\n");
	alarm(5);
	printf("Calling raise(SIGUSR1)\n");
	raise(SIGUSR1);
	printf("Calling raise(SIGUSR2)\n");
	raise(SIGUSR2);
}