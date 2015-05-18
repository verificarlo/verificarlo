//#######################################
//## Author: Eric Petit
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"
#include <emmintrin.h>


double AccSumVectIn(double * __restrict__ p, unsigned int n) {
	double mu, Ms, sigma, phi, factor;
	double q, t, tau, tau1, tau2;
	
	__m128d q_v,p_v,tau_v,sigma_v;
	double temp[2] __attribute__ ((aligned (16)));

	
	int i;
	
	mu = fabs(p[0]);
	for(i=1; i<n; i++) { if( fabs(p[i]) > mu ) mu = fabs(p[i]); }
	if(mu == 0.0) return(0.0);
	NextPowerTwo(Ms, n+2); 
	NextPowerTwo(sigma, mu); sigma *= Ms; 
	phi = URD_DBL*Ms;
	factor = EPS_DBL*Ms*Ms;
	
	t = 0.0;

	while(1) {
		
		tau = 0.0;
			
		/*for(i=0; i<n; i++) {
			q = (sigma + p[i]) - sigma;
			p[i] -= q; tau += q;
		}*/

		tau_v=_mm_setzero_pd();
	
		sigma_v=_mm_set1_pd(sigma);

	
		for(i=0; i<n; i+=2) {

			
			p_v=_mm_load_pd(&p[i]);
			
			//q = (sigma + p[i+2]) - sigma;
			//p[i+2] -= q; tau += q;	
			
	
			q_v = _mm_sub_pd(_mm_add_pd(sigma_v, p_v),sigma_v);
			p_v = _mm_sub_pd(p_v,q_v);
			
			_mm_store_pd(&p[i], p_v);
			//q = (sigma + p[i+3]) - sigma;
			//p[i+3] -= q; tau += q;				

			tau_v = _mm_add_pd(tau_v,q_v);
		
		
			
		}
		
		
		_mm_store_pd(&temp[0], tau_v); 
		tau= tau + temp[0] + temp[1];
		
								
		tau1 = t + tau;
		
		if((fabs(tau1) >= factor*sigma) || (sigma <= LBD_DBL)) {
			
			q = p[0]; for(i=1; i<n; i++) q += p[i];
			tau2 = tau - (tau1 - t);
			//printf("m1=%d\n",m);
			return(tau1 + (tau2 + q));
		}
		t = tau1;
		if(t == 0.0){
		//printf("m2=%d\n",m);
		  return(AccSumVectIn(p, n));
		} 
		sigma = phi*sigma;
	}
	//printf("m=%d\n",m);
	return(0.0);
}

