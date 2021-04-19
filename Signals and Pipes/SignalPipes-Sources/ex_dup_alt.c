#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    pid_t pid;
    int fd[2];

    if (pipe(fd) < 0) {
        perror("pipe() failed");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) < 0) {
        perror("fork() failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        char str[] = "Hello from your parent!\n";

        // parent - closing read end
        close(fd[0]); 

        // dip pipe write end file descriptor on stdin
        dup2(fd[1], STDOUT_FILENO);

        printf("%s\n", str);
    } else {
        char *args[] = {"cat", NULL};
        
        // child - close write end
        close(fd[1]);

        // dup pipe read end file descriptor on stdin
        dup2(fd[0], STDIN_FILENO);

        if (execvp(args[0], args) == -1)
        {
            perror("execvp() failed");
            exit(EXIT_FAILURE);
        }
    }
}