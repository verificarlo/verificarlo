#include <stdlib.h>
#include <stdio.h>

float a, b;

static void init_test_values(void) {
  int i;
  srand(0);
  a = (float)rand()/(float)RAND_MAX;
  b = (float)rand()/(float)RAND_MAX;
}


int main(void) {
  char buf[48];

  init_test_values();
  float c = a + b;
  sprintf(buf, "%.9f\n", c);
  // print only the last 5 digits
  printf("%s\n", buf + (11-5));
  return 0;
}
