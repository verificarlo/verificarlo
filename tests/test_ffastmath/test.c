#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>

int main (int argc, char ** argv)
{
    assert(argc == 2);

    /* Exemple taken from http://people.cs.clemson.edu/~steve/CPLSCMODS/123infty/floatexamples.new */

    double z = atof(argv[1]);

    double a = z/3.0;
    double b = z/5.0;
    double c = a+a+a;
    double d = b+b+b+b+b;
    double e = 1.0 - c;
    double f = 1.0 - d;
    double r = e / f;

    printf("%g %g\n", z, r);
}
