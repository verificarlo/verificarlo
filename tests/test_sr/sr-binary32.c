#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char * argv[]){
    assert(argc == 2);
    int p = atoi(argv[1]);
    float a = 1.25;
    float b = pow(2, -p);

    for (int i=0; i < ITERATIONS; i++) {
      printf("%.13a\n", a+b);
    }
    return 0;
}
