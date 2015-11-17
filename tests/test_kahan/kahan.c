/* Adapted from http://www.astro.umd.edu/~dcr/Courses/ASTR615/ps2sol/node1.html
** kahan.c
** =======
** Computes sums using the Kahan correction algorithm.
*/

#include <stdio.h>
#include <stdlib.h> /* for rand() */
#include <unistd.h> /* for getpid() */
#include <time.h> /* for time() */
#include <math.h>
#include <assert.h>
/* construct REAL "type," depending on desired precision */
#ifdef DOUBLE
#define REAL double
#else
#define REAL float
#endif

/* prototype and macro definition for absolute value function */

REAL ABS(REAL);
#ifdef DOUBLE
#define ABS(x) fabs(x)
#else
#define ABS(x) fabsf(x) /* single-precision version */
#endif

__attribute__ ((noinline))  REAL sum_kahan(const REAL f[],int N)
{
/*
** Implementation of the Kahan summation algorithm, taken from the
** following Wikipedia entry:
** http://en.wikipedia.org/wiki/Kahan_summation_algorithm
*/

    REAL sum = f[0];
    REAL c = 0.0,y,t;
    int i;

    for (i=1;i<N;i++) {
        y = f[i] - c;
        t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}

void fill_array(REAL f[],int N)
{
/* fill array with random values between 0 and 1 */

    int i;

    for (i=0;i<N;i++)
        f[i] = (REAL) rand()/RAND_MAX;
}

void show_sums(const REAL f[],int N)
{
    REAL rSumKahan = sum_kahan(f,N);

    assert(rSumKahan > 0.0); /* to ensure no division by zero */

#ifdef DOUBLE
    printf("Kahan = %.16e\n", rSumKahan);
#else
    printf("Kahan = %.7e\n", rSumKahan);
#endif

}

int main(int argc,char *argv[])
{
    printf("Kahan summation algorithm.\n");

/* check arguments */

    if (argc != 2) {
        fprintf(stderr,"Usage: %s array-size.\n",argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);

    if (N < 1) {
        fprintf(stderr,"Array size must be positive.\n");
        return 1;
    }

/* show relevant compile-time options, if any */

    printf("Compile-time options: ");
#ifdef DOUBLE
    printf("DOUBLE");
#else
    printf("none");
#endif
    printf(".\n");

/* seed the random number generator based on the current time and process IDs */

    //unsigned int seed =  time(NULL)%getpid() + getppid();
    unsigned int seed = 0;
    srand(seed);
    printf("Random number seed = %i.\n",seed);

/* assign space to array */

    REAL f[N];
    printf("Input: array size = %i.\n",N);

    fill_array(f,N);
    show_sums(f,N);
    return 0;
}
