#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mainFloat(float a, float b) {
  float sum = 0.0;
  for (int i = 0; i < 100; i++) {
    sum = a + b;
    printf("%.13a\n", sum);
  }
}

void mainDouble(double a, double b) {
  double sum = 0.0;
  for (int i = 0; i < 100; i++) {
    sum = a + b;
    printf("%.13a\n", sum);
  }
}

int main(int argc, char *argv[]) {
  if (strcmp(argv[3], "float")) {
    mainFloat(strtod(argv[1], NULL), strtod(argv[2], NULL));
  } else {
    mainDouble(strtod(argv[1], NULL), strtod(argv[2], NULL));
  }
}

