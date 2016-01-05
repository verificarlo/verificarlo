#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>

double f(double z) {
    return z-1.111111111112;
}

double g(double z) {
    return z-1.111111111112;
}

int main (void)
{
    double z = 1.111111111111 ;
    double r1 = f(z);
    printf("%a\n", r1);
    double r2 = g(z);
    fprintf(stderr, "%a\n", r2);
}
