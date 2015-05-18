//#######################################
//## Author: Eric Petit
//## adapted from Arnaud Tisserand C code
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "all_header.h"

double Sum2(double *p, unsigned int n) {
  double s, s_, c, t;
  unsigned int i;
  
  s = c = 0.0;
  for(i=0; i<n; ++i) {
    s_ = s + p[i];
    t = s_ - s;
    c += (s -  (s_ - t)) + (p[i] - t);
    s = s_;
  }
  return(s+c);
}