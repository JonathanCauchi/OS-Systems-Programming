#include <stdio.h>
#include <stdlib.h>

#include "concat.h"

void print_string(char *s) {
	if (s) printf("%s\n", s);
}

int main(int argc, char *argv[]) {
	char *s1 = "abra", *s2 = "cadabra";
	char *s = concatenate(s1, s2);
  	printf("%s + %s = %s\n", s1, s2, s);		

  	// show pointer before call to free
	printf("Pointer before free is %p\n", s);

	// free memory allocated by concatenate function
	free(s);

	// show pointer after call to free
	printf("Pointer after free is %p\n", s);

	// pass the dangling pointer to function
	print_string(s);
}