#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char message [] = "This is a system call test!\n";
    write(STDOUT_FILENO, (const void*)message, sizeof(message) - 1);
    exit(0);
}
