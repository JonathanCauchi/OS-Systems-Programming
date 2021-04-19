#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char *argv[]) {

	int shmid;
	char *shm;

	if ((shmid = shmget(0x666, 0, 0666)) == -1) {
		perror("shmget() failed:");
		exit(EXIT_FAILURE);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (char *)-1) {
		perror("shmat() failed:");
		exit(EXIT_FAILURE);
	}

	for(;;) {
		printf("Timer is : [%s]\n", shm);
		sleep(1);
	}
}