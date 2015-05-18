//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"
#include <emmintrin.h>


double AccSumParIn(double * __restrict__ p, unsigned int n) {
	double mu, Ms, sigma, phi, factor;
	double q, t, tau, tau1, tau2,taub, taubb;
	
	
	double temp[2] __attribute__ ((aligned (64)));

	
	int i;
	
	mu = fabs(p[0]);
	for(i=1; i<n; i++) { if( fabs(p[i]) > mu ) mu = fabs(p[i]); }
	if(mu == 0.0) return(0.0);
	NextPowerTwo(Ms, n+2); 
	NextPowerTwo(sigma, mu); 
	sigma *= Ms; 
	phi = URD_DBL*Ms;
	factor = EPS_DBL*Ms*Ms;
	
	t = 0.0;

	while(1) {
		
		tau = 0.0;
		taub = 0.0, taubb = 0.0;

		int chunk=100;

		int tid, nthreads;
		
		#pragma omp  parallel shared(tau,p,temp) private(i)
		{
		__m128d q_v,p_v,tau_v,sigma_v;
		double tau_tmp=0,qt=0;
		tau_v=_mm_setzero_pd();
	
		sigma_v=_mm_set1_pd(sigma);
		// tid = omp_get_thread_num();
 		// if (tid == 0)
    		//{
   		// nthreads = omp_get_num_threads();
    		// printf("Starting inner loop with %d threads\n",nthreads);
   		// }
		
		 #pragma omp for schedule (dynamic, chunk)
		for(i=0; i<n; i+=2) {
			p_v=_mm_load_pd(&p[i]);
					
			q_v = _mm_sub_pd(_mm_add_pd(sigma_v, p_v),sigma_v);
			p_v = _mm_sub_pd(p_v,q_v);
			
			_mm_store_pd(&p[i], p_v);
			
			tau_v = _mm_add_pd(tau_v,q_v);
		
		
			
		}
 		
		/*#pragma omp for schedule (dynamic, chunk)
		for(i=0; i<n; i+=4) {

			
			p_v=_mm_load_pd(&p[i]);
			
			qt = (sigma + p[i+2]) - sigma;
			p[i+2] -= qt; tau_tmp += qt;	
			
	
			q_v = _mm_sub_pd(_mm_add_pd(sigma_v, p_v),sigma_v);
			p_v = _mm_sub_pd(p_v,q_v);
			
			_mm_store_pd(&p[i], p_v);

			qt = (sigma + p[i+3]) - sigma;
			p[i+3] -= qt; tau_tmp += qt;				

			tau_v = _mm_add_pd(tau_v,q_v);
		
		
			
		}*/
		
		//printf("Thread=%d last did trip=%d\n",tid,i);
		#pragma omp critical
		{		
		_mm_store_pd(&temp[0], tau_v); 
		tau = tau+tau_tmp+temp[0] + temp[1];
		}
		
		}
		
		
								
		tau1 = t + tau;
		
		if((fabs(tau1) >= factor*sigma) || (sigma <= LBD_DBL)) {
			
			q = p[0]; for(i=1; i<n; i++) q += p[i];
			tau2 = tau - (tau1 - t);
			return(tau1 + (tau2 + q));
		}
		t = tau1;
		if(t == 0.0){
		  return(AccSumParIn(p, n));
		} 
		sigma = phi*sigma;
	}
	return(0.0);
}

