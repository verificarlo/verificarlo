#include <stdlib.h>
#include <stdio.h>

double a, b;

static void init_test_values(void) {
  int i;
  srand(0);
  a = (double)rand()/(double)RAND_MAX;
  b = (double)rand()/(double)RAND_MAX;
}


int main(void) {
  char buf[48];

  init_test_values();
  double c = a + b;
  sprintf(buf, "%.17f\n", c);
  // print only the last 5 digits
  printf("%s\n", buf + (19-5));
  return 0;
}
