#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *concatenate(char *s1, char *s2) 
{
  // make sure both are valid strings
  assert(s1 != NULL && s2 != NULL);

  // assume strings are null-terminated
  // get respective lengths
  size_t l1 = strlen(s1),
    l2 = strlen(s2);

  // allocate a zeroed memory chunk the cumulative size of the
  // two strings plus an additional character for null termination
  char *s = calloc(1, l1 + l2 + 1);
  if (!s) {
    perror ("calloc");
    exit(EXIT_FAILURE);
  }

  // copy strings into newly allocated buffer
  strcpy(s, s1);
  strcpy(s + l1, s2);

  return s;
}