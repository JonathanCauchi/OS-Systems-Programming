#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	const int blocks = 4;
	const int block_size_heap = 64 * 1024;
	const int block_size_amm = block_size_heap * 1024;

	char *block_list[blocks];

	printf("Allocation size: %d\n", block_size_heap);
	for (int j = 0; j < blocks; ++j) {
		block_list[j] = malloc(block_size_heap);
		printf("Address : %p, Break : %p\n", block_list[j], sbrk(0));
	}

	printf("Allocation size: %d\n", block_size_amm);
	for (int j = 0; j < blocks; ++j) {
		free(block_list[j]);
		block_list[j] = malloc(block_size_amm);
		printf("Address : %p, Break : %p\n", block_list[j], sbrk(0));
	}

	for (int j = 0; j < blocks; ++j)
		free (block_list[j]);
}