#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double Fdouble(double a){
	return a - 10;
}

float Ffloat(float a){
	return a - 10;
}

int main(int argc, char const *argv[])
{
	for (int i = 1; i < 111; i += 10){
		float f = i + 0.333;
		Ffloat(f);
		Ffloat(-f);
		double d = i + 0.333;
		Fdouble(d);
		Fdouble(-d);
	}
	return 0;
}