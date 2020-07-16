#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u_32;

int main(int argc, char const *argv[])
{
	if (argc < 2){
		fprintf(stderr, "./test [tolerance]\n");
		abort();
	}

	union {u_32 i; float f;} a_32;
	union {u_32 i; float f;} b_32;
	union {u_32 i; float f;} c_32;

	int tolerance = atoi(argv[1]) - 1;

	a_32.i = (((u_32)1 << 23)-1) - (((u_32)1 << (23-tolerance))-1) + ((u_32)1 << 30);

	b_32.i = (((u_32)1 << 23)-1) + ((u_32)1 << 30);

	c_32.f = b_32.f - a_32.f;

	printf("%a\n", c_32.f);

	return 0;
}
