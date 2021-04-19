#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid_p1, pid_p2;
    int fd[2];

    if (pipe(fd) < 0) {
        perror("pipe() failed");
        exit(EXIT_FAILURE);
    }

    if ((pid_p1 = fork()) < 0) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid_p1 == 0) {
        char *args[] = {"ps", "-eaf", NULL};
        
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);

        if (execvp(args[0], args) == -1)
        {
            perror("execvp() failed");
            exit(EXIT_FAILURE);
        }
    }

    if ((pid_p2 = fork()) < 0) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid_p2 == 0) {
        char *args[] = {"more", NULL};
        
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);

        if (execvp(args[0], args) == -1)
        {
            perror("execvp() failed");
            exit(EXIT_FAILURE);
        }
    }

    // parent process closes both pipes
    close(fd[0]);
    close(fd[1]);

    // wait for termination of last pipeline stages
    int status;
    waitpid(pid_p2, &status, 0);

    printf("Pipeline execution complete. \n");
}