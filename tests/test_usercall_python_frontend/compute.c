#include <interflop/interflop.h>

/* Repeated multiplication -- result grows quickly, making it sensitive to
 * both precision reduction (MCA noise) and exponent-range clipping (vprec). */
float compute_float(void) {
  float r = 3.25938657906f;
  float p = 4.05896796763f;
  for (int i = 0; i < 5; i++)
    r = r * p;
  return r;
}

double compute_double(void) {
  double r = 3.25938657906;
  double p = 4.05896796763;
  for (int i = 0; i < 5; i++)
    r = r * p;
  return r;
}
