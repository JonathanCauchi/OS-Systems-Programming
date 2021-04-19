#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char *argv[]) {

	int shmid;
	char *shm;

	if ((shmid = shmget(0x666, 256, IPC_CREAT | 0666)) == -1) {
		perror("shmget() failed:");
		exit(EXIT_FAILURE);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (char *)-1) {
		perror("shmat() failed:");
		exit(EXIT_FAILURE);
	}

	int timer = 0;

	for(;; ++timer) {
		sprintf(shm, "%2d:%2d:%2d", (timer / 3600) % 24, (timer / 60) % 60, timer % 60);
		sleep(1);
	}
}