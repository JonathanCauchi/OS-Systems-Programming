#include <stdio.h>
#include <stdlib.h>

#include "concat.h"

char *concatenate4(char *s1, char *s2, char *s3, char *s4) {
	char *s12 = concatenate(s1, s2);
	char *s34 = concatenate(s3, s4);
	char *s = concatenate(s12, s34);

	return s;
}

int main(int argc, char *argv[]) {
  char *s1 = "You ", *s2 = "shall ", 
       *s3 = "not ", *s4 = "pass.";
 
  // concatenate the four strings 
  char *s = concatenate4(s1, s2, s3, s4);
  
  // print output
  printf("%s + %s + %s + %s = %s\n", s1, s2, s3, s4, s);

  // free memory block returned by concatenate4
  free(s);
}