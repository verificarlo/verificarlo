//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"
#include <emmintrin.h>


double FastAccSumOpt_unroll2_In(double *, unsigned int ); 

double FastAccSumOpt_unroll2(double *p, unsigned int n) {
  double *q, r;
  
  q = (double *)malloc(sizeof(double)*n);
  memcpy(q, p, sizeof(double)*n);
  r = FastAccSumOpt_unroll2_In(q, n);
  free(q);
  return r;
}


double FastAccSumOpt_unroll2_In(double *p, unsigned int n) {
	double res=0,abssum=0,sum=0,t=0,t_prime=0;
	int i=0;
	double sigma=0 , sigmaO=0 , sigma_prime=0 , T=0 , q=0 , tau=0, phi=0, tau2=0,u=0;
	double lim=ETA_DBL/URD_DBL;
	
	
	
	for(i=0;i<n;i++)
	{
		abssum+=fabs(p[i]);
		sum+=p[i];
	}
	
	T=abssum/(1-((double)n*URD_DBL));
	
	if ((T) <= (lim)) return sum;
	
	t_prime=0;
	//int m=0;
	do
	{
		//m++;
		sigmaO=(double)(2.*T/(1.-(3.*n+1.)*URD_DBL));
		sigma=sigmaO;
		for(i=0;i<n;i+=2)
		{
			sigma_prime= sigma+p[i];
			q=sigma_prime-sigma;
			p[i]=p[i]-q;
			sigma=sigma_prime+p[i+1];
			q=sigma-sigma_prime;
			p[i+1]=p[i+1]-q;
		}
		
		
		
		tau=sigma-sigmaO;
		//printf("tau2=%f\n",tau);
		t=t_prime;
		t_prime=t+tau;
		if (t_prime==0) 
			return res=FastAccSumOpt_unroll2_In(p,n);
		
		q=sigmaO/(2*URD_DBL);
		u=fabs(q/(1-URD_DBL)-q);
		
		phi= ((2*n*(n+2)*URD_DBL)*u)/(1-5*URD_DBL);
		
		T=fmin(     ((3./2.+4*URD_DBL)*(n*URD_DBL))*(sigmaO)    ,   2*n*URD_DBL*u  );
	}while(!((fabs(t_prime)>=phi)|| (4*T<=lim)) );
	//printf("m=%d\n",m);
	
	tau2=(t-t_prime)+tau;
	
	sum=0;
	for(i=0;i<n;i++)
	{
		sum+=p[i];
	}
	
	res=t_prime+(tau2+(sum));
	
	return res;
}

double FastAccSumOpt_vect2_In(double *p, unsigned int n) {
	double res=0,abssum=0,sum=0,t=0,t_prime=0;
	int i=0;
	double sigma=0 , sigmaO=0 , sigma_prime=0 , T=0 , q=0 , tau=0, phi=0, tau2=0,u=0;
	double lim=ETA_DBL/URD_DBL;
	
	
	
	for(i=0;i<n;i++)
	{
		abssum+=fabs(p[i]);
		sum+=p[i];
	}
	
	T=abssum/(1-((double)n*URD_DBL));
	
	if ((T) <= (lim)) return sum;
	
	t_prime=0;
	//int m=0;
	do
	{
		//m++;
		sigmaO=(double)(2.*T/(1.-(3.*n+1.)*URD_DBL));
		sigma=sigmaO;


		//double sigma_t[2],sigma_prime_t[2], p_t[2], q_t[2];
		__m128d sigma_v,sigma_prime_v, p_v, q_v;
		double temp[2] __attribute__ ((aligned (16)));
		
		

		/*//sigma_t[0]= sigma;
		//sigma_t[1]= sigma+p[0];		
		sigma_v=_mm_set_pd(sigma+p[0],sigma);
		
		
		
		int N=n-2;
		
		for(i=0;i<N;i+=2)
		{
				
			//p_t[0]=p[i];
			//p_t[1]=p[i+1];
			p_v=_mm_load_pd(&p[i]);
			

			//sigma_prime_t[0]= sigma_t[0]+p_t[0];
			//sigma_prime_t[1]= sigma_t[1]+p_t[1];
			sigma_prime_v=_mm_add_pd(sigma_v,p_v);
				
			

				
			_mm_store_pd(&temp[0], sigma_prime_v);
			//..#1
			
			

			//q_t[0]=sigma_prime_t[0]-sigma_t[0];
			//q_t[1]=sigma_prime_t[1]-sigma_t[1];
			q_v=_mm_sub_pd(sigma_prime_v,sigma_v);

			//#1..
			//sigma_t[0]=sigma_prime_t[1];
			//sigma_t[1]=sigma_prime_t[1]+p[i+2];
			sigma_v=_mm_set_pd(temp[1]+p[i+2],temp[1]);	

			//p_t[0]=p_t[0]-q_t[0];
			//p_t[1]=p_t[1]-q_t[1];
			p_v=_mm_sub_pd(p_v,q_v);

			//p[i]=p_t[0];
			//p[i+1]=p_t[1];
			_mm_store_pd(&p[i], p_v);


								

				
			

		}
		
		p_v=_mm_load_pd(&p[i]);
		sigma_prime_v=_mm_add_pd(sigma_v,p_v);
		_mm_store_pd(&temp[0], sigma_prime_v);
		q_v=_mm_sub_pd(sigma_prime_v,sigma_v);
		p_v=_mm_sub_pd(p_v,q_v);
		_mm_store_pd(&p[i], p_v);*/

		int N=n-2; //=>peeling of  the 2 last iteration

		__m128d zero_v=_mm_setzero_pd ();//=>for shuffle purpose
		
		//init
		p_v=_mm_load_pd(p);
		sigma_v=_mm_set_pd(sigma+p[0],sigma);

		for(i=0;i<N;i+=2)
		{
				
		
	
			sigma_prime_v=_mm_add_pd(sigma_v,p_v);
				
			q_v=_mm_sub_pd(sigma_prime_v,sigma_v);

			p_v=_mm_sub_pd(p_v,q_v);
			_mm_store_pd(&p[i], p_v);	
		
			p_v=_mm_load_pd(&p[i+2]);
			__m128d s1 = _mm_unpackhi_pd(sigma_prime_v,sigma_prime_v);
			__m128d s2 = _mm_unpacklo_pd(zero_v,p_v);
			sigma_v=_mm_add_pd(s1,s2);

			
		
			

		}
		
		p_v=_mm_load_pd(&p[i]);
		sigma_prime_v=_mm_add_pd(sigma_v,p_v);
		_mm_store_pd(&temp[0], sigma_prime_v);
		q_v=_mm_sub_pd(sigma_prime_v,sigma_v);
		p_v=_mm_sub_pd(p_v,q_v);
		_mm_store_pd(&p[i], p_v);
	

		tau=temp[1]-sigmaO;
		//printf("tau2=%f\n",tau);
		t=t_prime;
		t_prime=t+tau;
		if (t_prime==0) 
			return res=FastAccSumOpt_unroll2_In(p,n);
		
		q=sigmaO/(2*URD_DBL);
		u=fabs(q/(1-URD_DBL)-q);
		
		phi= ((2*n*(n+2)*URD_DBL)*u)/(1-5*URD_DBL);
		
		T=fmin(     ((3./2.+4*URD_DBL)*(n*URD_DBL))*(sigmaO)    ,   2*n*URD_DBL*u  );
	}while(!((fabs(t_prime)>=phi)|| (4*T<=lim)) );
	//printf("m=%d\n",m);
	
	tau2=(t-t_prime)+tau;
	
	sum=0;
	for(i=0;i<n;i++)
	{
		sum+=p[i];
	}
	
	res=t_prime+(tau2+(sum));
	
	return res;
}
