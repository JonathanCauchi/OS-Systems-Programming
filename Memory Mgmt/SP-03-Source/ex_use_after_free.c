#include <stdio.h>
#include <stdlib.h>

#include "concat.h"

int main(int argc, char *argv[]) {
	// define strings s1 and s2
	char *s1 = "abra", *s2 = "cadabra";
  
	// call concatenate; returns a pointer to the newly concatenated string
	char *s = concatenate(s1, s2);
  
	// free memory allocated by concatenate function
	free(s);

	// print output
	printf("%s + %s = %s\n", s1, s2, s);		
}