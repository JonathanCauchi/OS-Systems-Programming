#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("PID is [%d]; parent PID is [%d]\n"
        ,getpid()
        ,getppid()
    );

    return 0;
}