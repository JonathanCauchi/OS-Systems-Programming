#include "pre.h"

#define TEST "This is a test!"

int main(int argc, char** argv) {
  /* A comment - the preprocessor will cull this line	*/	

  /* Conditional compilation based on whether INVERT is defined */
#ifdef INVERT
  int num_1 = TWO,
      num_2 = ONE;
#else
  int num_1 = ONE,
      num_2 = TWO;
#endif

  int num_3 = MIN(num_1, num_2);
  int num_4 = MAX(num_1, num_2);

  int add_2 = add_two(num_3, num_4);
  int sub_2 = sub_two(num_3, num_4);

  /* While we're at it, let's define a string */
  char *str_1 = TEST;

  return 0;
}