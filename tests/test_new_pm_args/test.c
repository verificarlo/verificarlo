#include <stdio.h>

int main(void) {
  float a = 0.1f;
  float b = 0.01f;
  float c = a * b;
  printf("%.13a\n", c);
  return 0;
}
