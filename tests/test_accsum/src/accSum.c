//#######################################
//## Author: Eric Petit
//## adapted from Arnaud Tisserand C code
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"
#include <emmintrin.h>

double AccSum(double *p, unsigned int n) {
  double *q, r;
  
  q = (double *)malloc(sizeof(double)*n);
  memcpy(q, p, sizeof(double)*n);
  r = AccSumIn(q, n);
  free(q);
  return r;
}

double GAccSum(double *p, unsigned int n) {
  double *q, r;
  
  q = (double *)malloc(sizeof(double)*n);
  memcpy(q, p, sizeof(double)*n);
r = GAccSumIn(q, n);
  free(q);
  return r;
}


double AccSumIn(double *p, unsigned int n) {
	double mu, Ms, sigma, phi, factor;
	double q, t, tau, tau1, tau2;
	int i;
	
	mu = fabs(p[0]);
	for(i=1; i<n; i++) { if( fabs(p[i]) > mu ) mu = fabs(p[i]); }
	if(mu == 0.0) return(0.0);
	NextPowerTwo(Ms, n+2); /*Ms=NextPowerTwo(n+2)*/
	NextPowerTwo(sigma, mu); sigma *= Ms; /*sigma=Ms*NextPowerTwo(mu)*/
	phi = URD_DBL*Ms;
	factor = EPS_DBL*Ms*Ms;
	
	t = 0.0;
	
	while(1) {
	

		tau = 0.0;
		for(i=0; i<n; i++) {
			q = (sigma + p[i]) - sigma;
			p[i] -= q; tau += q;
		}
		tau1 = t + tau;
		
		if((fabs(tau1) >= factor*sigma) || (sigma <= LBD_DBL)) {
			q = p[0]; for(i=1; i<n; i++) q += p[i];
			tau2 = tau - (tau1 - t);
			return(tau1 + (tau2 + q));
		}
		t = tau1;
		if(t == 0.0) return(AccSumIn(p, n)); 
		sigma = phi*sigma;
	}
	return(0.0);
}


double GAccSumIn(double *p, unsigned int n) {
	double mu, Ms, sigma, phi, factor;
	double q, t, tau, tau1, tau2;
	int i;
	
	mu = fabs(p[0]);
	for(i=1; i<n; i++) { if( fabs(p[i]) > mu ) mu = fabs(p[i]); }
	if(mu == 0.0) return(0.0);
	NextPowerTwo(Ms, n+2); /*Ms=NextPowerTwo(n+2)*/
	NextPowerTwo(sigma, mu); sigma *= Ms; /*sigma=Ms*NextPowerTwo(mu)*/
	phi = URD_DBL*Ms;
	factor = EPS_DBL*Ms*Ms;
	
	t = 0.0;
	
	while(1) {
	

		tau = 0.0;
		for(i=0; i<n; i++) {
			q = (sigma + p[i]) - sigma;
			p[i] -= q; tau += q;
		}
		tau1 = t + tau;
		
		if((fabs(tau1) >= factor*sigma) || (sigma <= LBD_DBL)) {
			q = p[0]; for(i=1; i<n; i++) q += p[i];
			tau2 = tau - (tau1 - t);
			return(tau1 + (tau2 + q));
		}
		t = tau1;
		if(t == 0.0) return(AccSumIn(p, n)); 
		sigma = phi*sigma;
	}
	return(0.0);
}

void ExtractVector(double *p, double *tau, double sigma, unsigned int n) {
	int i;
	double q;
	
	for(i=0; i<n; i++) {
		q = (sigma + p[i]) - sigma;
		p[i] -= q; *tau += q;
	}
	
	//  return(r);
}

