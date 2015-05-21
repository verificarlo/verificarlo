//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"
#include<math.h>
#define DEBUG 0


void init(double **P, int SIZE)
{
	int i;
	*P = (double *)malloc(sizeof(double)*SIZE);
	for(i=0; i<SIZE; i++)
    {
		(*P)[i] = 0;
		//printf("initial p[%d]=%1.16f\n",i,(*P)[i]);
		
    }
}



double GenSum(double *x, double *C, unsigned int n, double c) {
	unsigned int i, j, n2 = n/2;
	double b, s;
	
	b = log2(c);

	if (DEBUG>0) printf("log2(Cin)=%f\n",b);

	x[0] = (2*rand_double(1)-1);
	x[1] = ldexp( 2*rand_double(1)-1, nearbyint(b)+1 );

	for(i=2; i<n2; i++)
		x[i] = ldexp( rand_unif(-1.0, 1.0), nearbyint(rand_unif(0.0, 1.0)*b) );
	


	if (DEBUG > 1) for(i=0;i<n;i++) printf("init x = %.17f\n",x[i]);


	for(i=n2; i<n; i++)
		x[i] = ldexp( rand_unif(-1.0, 1.0), nearbyint(b-(i-n2)*b/(n-1-n2)) ) - GAccSum(x, i);
	

	if (DEBUG > 1) for(i=0;i<n;i++) printf("first rank x = %.17f\n",x[i]);
		

	for(i=1; i<n; i++)
		if((j = rand_uint(i)) != i) { b = x[j]; x[j] = x[i]; x[i] = b; }
	


	if (DEBUG > 0) for(i=0;i<n;i++) printf("final x= %.17f\n",x[i]);
	

	b = fabs(x[0]);
	for(i=1; i<n; i++) b += fabs(x[i]);

	s = GAccSum(x, n);
	
	if (DEBUG > 0) printf("AccSum=%.17f\n",s);	

	*C = b / fabs(s);
	
	if (DEBUG > 1) for(i=0;i<n;i++) printf("final2 x= %.17f\n",x[i]);


	if (DEBUG > 1) printf("C=%.17f\n",*C);	
	
	if (DEBUG > 1) for(i=0;i<n;i++) x[i]=log2(1.01010101*i+1)*(0.1+((-1)*(i%2)));//verif meme nb elem, i pour verif meme elem
	

	if (DEBUG > 3) *C = 3;
	
	return s;
}

double GenSum_inFile(double *x, double *C, unsigned int n, int c_exp)
{
	
	char* filename=malloc(45*sizeof(char));
	double b=0,s,c=1;
	int i,out_fd;
	
	
	sprintf(filename,"gensumFile.C%d.n%d.data",c_exp,n);
	
	if (DEBUG>0) printf("create: %s\n",filename);
	
	out_fd=open(filename, O_RDWR|O_CREAT,S_IRWXU);
		
	for(i=0;i<c_exp;i++) c*=10.;
	
	
	s=GenSum(x,C,n,c);
	
	if (DEBUG>0) printf("sum s=%f generated for C=%.1e, dumping out in file...\n",s,c);
	
	i=write(out_fd,x,n*sizeof(double));
	
	for(i=0; i<n; i++) b += fabs(x[i]);

	s = GAccSum(x, n);
	
	
	*C = b / fabs(s);
	
	close(out_fd);
	
	if (DEBUG>0) printf("file %s completed and closed\n",filename);
	
	if (DEBUG>0) printf("returned accsum =%f\n",s);
	
	return s;
	
}
