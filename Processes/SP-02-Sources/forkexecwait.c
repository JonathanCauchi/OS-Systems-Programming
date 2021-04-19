#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        if (execlp("ps", "ps", "-f", NULL)) {
            perror("execlp() failed");
            exit(EXIT_FAILURE);
        }

        printf("This string should never get printed\n");
    }

    printf("Parent process after fork(); child PID is [%d]\n", pid);

    int status;
 
    if (wait(&status) == -1)
        perror("wait() failed");

    printf("Parent process after wait()\n");
}
