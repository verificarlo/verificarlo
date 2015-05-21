//##############################################
//## Author: Arnaud Tisserand and David Parello
//##############################################

#include "all_header.h"


double DDSum(double *P, unsigned int n) {
  double  r_h, r_l, t_h, t_l;
  int i;
  
  r_h = 0.0; r_l = 0.0;
  for(i=0; i<n; i++) {
    /* (r_h, r_l) = (r_h, r_l) + P[i] */
    TwoSum(t_h, t_l, r_h, P[i]);
    t_l += r_l;
    FastTwoSum(r_h, r_l, t_h, t_l);
  }
  return(r_h);
}

double DDSumBis(double *P, unsigned int n) {
  double  r_h, r_l, t_h, t_l, t0, t1, t2, t3, t4;
  int i;
  
  r_h = 0.0; r_l = 0.0;
  for(i=0; i<n; i++) {
    /* (r_h, r_l) = (r_h, r_l) + P[i] */
    /* TwoSum(t_h, t_l, r_h, P[i]); t_l += r_l;*/
    t_h = r_h + P[i];
    t0 = t_h - r_h;
	t3 = P[i] - t0;
    t1 = t_h - t0;
    t2 = r_h - t1;
	t4 = t2 + t3;
   
	 t_l = t4 + r_l;
	 
    /* FastTwoSum(r_h, r_l, t_h, t_l); */
    r_h = t_h + t_l;
    t0 = t_h - r_h;
    r_l = t0 + t_l;
  }
  return(r_h);
}
