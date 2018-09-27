#include <stdlib.h>
#include <stdio.h>

#ifdef DOUBLE
#define REAL double
#define OUTPUT "%.17e"
#else
#define REAL float
#define OUTPUT "%.7e"
#endif

int main(int argc, char *argv[]) {
  int n = 10;

  REAL sum = 0.0;
  
  for (int i = 0; i < n; i++)
    sum += 0.1*i + 0.01*i*i;

  printf(OUTPUT"\n",sum);
}
