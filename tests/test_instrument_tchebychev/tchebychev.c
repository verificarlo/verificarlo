#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>

double coefs[] = {
    - 200.,
    6600.,
    - 84480.,
    549120.,
    - 2050048.,
    4659200.,
    - 6553600.,
    5570560.,
    -2621440.,
    524288.
};


int main (int argc, char ** argv)
{
    assert(argc == 2);
    double z = atof(argv[1]);

    double r = 1.0;

    double z2 = z*z;

    double p = z2;

    for (int i = 0; i < 10; i++) {
        r += coefs[i]*p;
        p *= z2;
    }

    printf("%g %g\n", z, r);
}
