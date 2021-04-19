#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("Running execvpe()...\n");
    
    // make sure we have the correct number of args
    if (argc < 2) {
        printf("No program image supplied!\n");
        exit(0);
    }

    // environment
    char * const env[] = {
        "HOME=/user/wanpanman", "USER=wanpanman", NULL 
    };

    // call exec with one less argument (arg[0])
    if (execvpe(argv[1], argv + 1, env)) {
        perror("execvpe failed:");
        exit(1);
    }

    printf("This message will never show.\n");
}