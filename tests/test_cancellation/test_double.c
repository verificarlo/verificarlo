#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned long long int u_64;

int main(int argc, char const *argv[])
{
	if (argc < 2){
		fprintf(stderr, "./test [tolerance]\n");
		abort();
	}

	union {u_64 i; double f;} a_64;
	union {u_64 i; double f;} b_64;
	union {u_64 i; double f;} c_64;

	int tolerance = atoi(argv[1]) - 1;

	a_64.i = (((u_64)1 << 52)-1) - (((u_64)1 << (52-tolerance))-1) + ((u_64)1 << 62);

	b_64.i = (((u_64)1 << 52)-1) + ((u_64)1 << 62);

	c_64.f = b_64.f - a_64.f;

	printf("%la\n", c_64.f);

	return 0;
}
