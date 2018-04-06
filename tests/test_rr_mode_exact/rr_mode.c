#include <stdio.h>

/* exact operation: 0x1.0000000000000p+19 + -0x1.0000002000000p+46 = -0x1.0000000000000p+46 */
int main(int argc, char * argv[]){
#ifdef DOUBLE
  double a = 0x1.0000000000000p+19;
  double b = -0x1.0000002000000p+46;
  double c; // -0x1.0000000000000p+46;
  printf("%.13a\n", a+b);
#else
  float a = 0x1.000000p+34;
  float b =-0x1.001000p+46;
  float c; // -0x1.000000p+46
  printf("%.6a\n", a+b);
#endif
  return 0;
}
