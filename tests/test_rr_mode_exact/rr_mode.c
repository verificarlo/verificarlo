#include <stdio.h>

int main(int argc, char * argv[]){
  #ifdef DOUBLE
    /* exact operation: 0x1.0000000000000p+19 + 0x1.0000000000000p+46 = 0x1.0000002000000p+46 -*/
    double a = 0x1.0000000000000p+19;
    double b = 0x1.0000000000000p+46 ;
    printf("%.13a\n", a+b);
  #endif
  #ifdef DOUBLE_POW2
    /* exact operation that results in a power of two: 1.0 + 0.0 = 1.0 */
    double a = 1.0;
    double b = 0.0;
    printf("%.13a\n", a+b);
  #endif
  #ifdef FLOAT
    /* exact operation:  0x1.000000p+30 + 0x1.000000p+34 */
    float a = 0x1.000000p+30;
    float b = 0x1.000000p+34;
    printf("%.6a\n", a+b);
  #endif
  #ifdef FLOAT_POW2
    /* exact operation that results in a power of two: 1.0f + 0.0f = 1.0f */
    float a = 1.0f;
    float b = 0.0f;
    printf("%.6a\n", a+b);
  #endif
    return 0;
}
