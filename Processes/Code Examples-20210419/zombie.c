#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) 
{
    pid_t p = fork();

    if (p != 0) 
    {
        pause(); // Suspend main task.
    }
    else
    {
        sleep(3); // Just let the child live for some time before becoming a zombie
    }

    return 0;
}
