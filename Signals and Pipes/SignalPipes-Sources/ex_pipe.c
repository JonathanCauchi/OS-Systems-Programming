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
        write(fd[1], str, sizeof(str)); // write end
    } else {
        char str[256];
        
        // child - close write end
        close(fd[1]); 
        int bytes_read = read(fd[0], str, sizeof(str)); // read end 
        write(STDOUT_FILENO, str, bytes_read); 
    }
}