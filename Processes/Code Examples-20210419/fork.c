#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int ret = fork();

    if (ret == 0)
    {
        printf("Child process with pid %d\n", getpid());
        return 5;
    }
    else if (ret > 0)
    {
        printf("Parent process of child %d (my pid: %d)\n", ret, getpid());
        printf("Child process exited: %d\n", statloc);
    }   
    else
    {
        printf("Error forking\n");
        exit(0);
    }

    printf("Parent exiting\n");
}
