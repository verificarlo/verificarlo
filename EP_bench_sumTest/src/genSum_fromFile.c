//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

//#define VERBOSE_TEST 0
#include "all_header.h"
#include "bench.conf"


double GenSum(double *, double *, unsigned int , double ); 

double genSum_fromFile(double *x, double *C, unsigned int n, int c_exp) 
{
	double b=0, s;
	int i=0,j=0,in_fd,lim;
	char* filename=malloc(65*sizeof(char));
	
	if (n>=10000)
		lim=10000;
	else if (n>=1000)
		lim=1000;
	else if (n>=100)
		lim=100;
	else
		{perror("Can't guarantee correct cond. number, abort!"); exit(1);}	
	
	sprintf(filename,"./DATAin/gensumFile.C%d.n%d.data",c_exp,lim);
	if (VERBOSE_TEST>0) printf("----  using %s  \n",filename);
	in_fd=open(filename, O_RDONLY,S_IRUSR);
		
	if (n>=lim) 
	{
		i=read(in_fd,x,lim*sizeof(double));
		for(j=0 ; j<((int)floor(n/lim)) ; j++) memcpy(x+j*lim,x,lim*sizeof(double));
	}
	
	lseek(in_fd,0,SEEK_SET);
	i=read(in_fd,x+((int)(floor(n/lim))-1)*lim,(n%lim)*sizeof(double));


	for(i=0; i<n; i++) b += fabs(x[i]);
	s = AccSum(x, n);
	*C = b / fabs(s);
	
	return s;
}
