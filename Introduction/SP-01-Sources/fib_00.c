#include <stdio.h>

int fib(int n) {
	const int f[] = {0, 1, 1};
	return (n < 3) ? f[n] : fib(n-1) + fib(n-2);
}

int main(int argc, char **argv) {
	int f40 = fib(40);
	printf("F(40) = %d\n", f40);
	return 0;
}