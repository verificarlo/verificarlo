//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"

#define URD_DBL_times_2 2.22044604925031308084726333618164062500000000000000000000000000e-16
#define INV_URD_DBL_times_2  4.50359962737049600000000000000000000000000000000000000000000000e15
#define URD_DBL_times_4 4.44089209850062616169452667236328125000000000000000000000000000e-16
#define URD_DBL_times_5 5.55111512312578270211815834045410156250000000000000000000000000e-16
#define c1_minus_URD_DBL 0.99999999999999988897769753748434595763683319091796875
#define c1_minus_URD_DBL_times_5 0.999999999999999444888487687421729788184165954589843750000000000
#define c3_over_c2_plus_URD_DBL_times_4 1.50000000000000044408920985006261616945266723632812500000000000
#define ETA_OVER_EPS 4.45014771701440276618046543466480812843843196092466366110665483e-308

double FastAccSumbIn(double *, unsigned int ); 

double FastAccSumb(double *p, unsigned int n) {
  double *q, r;
  
  q = (double *)malloc(sizeof(double)*n);
  memcpy(q, p, sizeof(double)*n);
  r = FastAccSumbIn(q, n);
  free(q);
  return r;
}


double FastAccSumbIn(double *p, unsigned int n) {
	double res=0,abssum=0,sum=0,t=0,t_prime=0;
	int i=0;
	double sigma=0 , sigmaO=0 , sigma_prime=0 , T=0 , q=0 , tau=0, phi=0, tau2=0,u=0;

	printf("toto");	
	
	
	for(i=0;i<n;i++)
	{
		abssum+=fabs(p[i]);
		sum+=p[i];
	}
	
	T=abssum/(1-((double)n*URD_DBL));
	
	if ((T) <= (ETA_OVER_EPS)) return sum;
	
	t_prime=0;
	
	do
	{
		sigmaO=(double)(2.*T/(1.-(3.*n+1.)*URD_DBL));
		sigma=sigmaO;
		for(i=0;i<n;i++)
		{
			sigma_prime= sigma+p[i];
			q=sigma_prime-sigma;
			p[i]-=q;
			sigma=sigma_prime;
		}
		
		tau=sigma_prime-sigmaO;
		t=t_prime;
		t_prime+=tau;
		if (t_prime==0) 
			return res=FastAccSumbIn(p,n);
		
		q=sigmaO*INV_URD_DBL_times_2;
		u=fabs(q/(c1_minus_URD_DBL)-q);
		
		phi= ((2*n*(n+2)*URD_DBL)*u)/(c1_minus_URD_DBL_times_5);
		
		T=fmin(     ((c3_over_c2_plus_URD_DBL_times_4)*(n*URD_DBL))*(sigmaO)    ,   n*URD_DBL_times_2*u  );
	}while(!((fabs(t_prime)>=phi)|| (4*T<=ETA_OVER_EPS)) );
	
	
	tau2=(t-t_prime)+tau;
	
	sum=0;
	for(i=0;i<n;i++)
	{
		sum+=p[i];
	}
	
	res=t_prime+(tau2+(sum));
	
	return res;
}
