#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	const int blocks = 50;
	const int block_size = 64 * 1024;
	char *block_list[blocks];

	for (int j = 0; j < blocks; ++j)
	{
		block_list[j] = malloc(block_size);
		printf("Current break is at %p\n", sbrk(0));
	}

	for (int j = 0; j < blocks; ++j)
		free (block_list[j]);
}