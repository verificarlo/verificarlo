#include <stdio.h>
#include <stdlib.h>

#define bit(A, i) ((A >> i) & 1)

typedef unsigned long long int u_64;
typedef unsigned int u_32;

static void print_double(double d) {
  u_64 a = *((u_64 *)&d);

  for (int i = 63; i >= 0; i--)
    (i == 63 || i == 52) ? printf("%lld ", bit(a, i))
                         : printf("%lld", bit(a, i));

  printf("\n");
}

int main(int argc, char const *argv[])
{
	double zero = 0.0;

	double a = zero + zero;

  print_double(zero);
  printf("\t\t\t\t + \t\t\t\t\n");
  print_double(zero);
  printf("\t\t\t\t = \t\t\t\t\n");
	print_double(a);

	printf("\n%la + %la = %la\n", zero, zero, a);

	return 0;
}
