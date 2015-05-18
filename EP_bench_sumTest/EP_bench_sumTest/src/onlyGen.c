//######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#####################################

#include "all_header.h"

#define DEBUG 0

//#define N 1000 //don't change this value without knowledge, need to remove dependencie from other file

int main(int argc, char *argv[])
{

	int i,c_exp,n;
	double *x,*tmp;
	double C,s=0,chk_s=0;

	
	
	if (argc < 3) { printf("usage: <cond_nb> <nb_elem>\n\n"); exit(-1); }
	
	c_exp = atoi(argv[1]);
	n= atoi(argv[2]);
	
	srand(3);
	init(&x, n);
	
	s=GenSum_inFile(x, &C, n, c_exp);

	if (DEBUG>0) printf("open file for checking...\n");
	
	char* filename=malloc(45*sizeof(char));
	sprintf(filename,"gensumFile.C%d.n%d.data",c_exp,n);
	int out_fd=open(filename, O_RDONLY,S_IRUSR);
	
	if (DEBUG>0) printf("file opened\n");
	
	init(&tmp, n);
	
	if (DEBUG>0) printf("structure initialized\n");
	
	read(out_fd,tmp,n*sizeof(double));
	
	if (DEBUG>0)  printf("structure comleted\n");

	
	chk_s= GAccSum(tmp, n) ;
	
	close(out_fd);
	
	if (DEBUG>0) printf("final accsum= %f\n",chk_s);
	
	return 0;
}
