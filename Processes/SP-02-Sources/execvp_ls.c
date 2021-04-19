#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("Running execvp()...\n");
    
    char * const args[] = {
        "ls", "-la", NULL   
    }; 

    if (execvp("ls", args)) {
        perror("execvp failed:");
        exit(1);
    }

    // never executes
    printf("This message will never be shown.\n");
}