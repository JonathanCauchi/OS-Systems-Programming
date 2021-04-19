#include <stdio.h>

int main(int argc, char **argv) {
	int sum = 0;
	
	for (int index=0; index < 1000; ++index)
		sum += index; 

	printf("Sum is: %d\n", sum);
}