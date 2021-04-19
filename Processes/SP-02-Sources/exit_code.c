#include <stdlib.h>

int gimme_exit_code(int exitcode) {
	if (exitcode)
		return exitcode;
}

int main(int argc, char **argv) {
	if (argc >= 2) return gimme_exit_code(atoi(argv[1]));
}