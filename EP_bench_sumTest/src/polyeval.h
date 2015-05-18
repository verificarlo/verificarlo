//#######################################
//## Author: Arnaud Tisserand
//#######################################

#include <math.h>
#include "dp_tools.h"
#include "sum.h"

/* ----------------------------------------------------------------- */
/* --- Some macros needed for quad-double computations --------------*/
/* ----------------------------------------------------------------- */

#define Renorm5(c0, c1, c2, c3, c4) { \
  double s0, s1, s2 = 0.0, s3 = 0.0; \
  FastTwoSum(s0, c4, c3, c4); \
  FastTwoSum(s0, c3, c2, s0); \
  FastTwoSum(s0, c2, c1, s0); \
  FastTwoSum(c0, c1, c0, s0); \
  s0 = c0; \
  s1 = c1; \
  FastTwoSum(s0, s1, c0, c1); \
  if (s1 != 0.0) { \
    FastTwoSum(s1, s2, s1, c2); \
    if (s2 != 0.0) { \
      FastTwoSum(s2, s3, s2, c3); \
      if (s3 != 0.0) { s3 += c4; } \
      else { s2 += c4; } \
    } else { \
      FastTwoSum(s1, s2, s1, c3); \
      if (s2 != 0.0) { FastTwoSum(s2, s3, s2, c4); } \
      else { FastTwoSum(s1, s2, s1, c4); }\
    } \
  } else { \
    FastTwoSum(s0, s1, s0, c2); \
    if (s1 != 0.0) { \
      FastTwoSum(s1, s2, s1, c3); \
      if (s2 != 0.0) { FastTwoSum(s2, s3, s2, c4); } \
      else { FastTwoSum(s1, s2, s1, c4); } \
    } else { \
      FastTwoSum(s0, s1, s0, c3); \
      if (s1 != 0.0) { FastTwoSum(s1, s2, s1, c4); } \
      else { FastTwoSum(s0, s1, s0, c4); } \
    } \
  } \
  c0 = s0; \
  c1 = s1; \
  c2 = s2; \
  c3 = s3; \
}

/* quad-double = quad-double + double (r = a + b) */
#define Add_QD_d(r, a, b) { \
  double e; \
  TwoSum(r[0], e, a[0], b); \
  TwoSumSafe(r[1], e, a[1], e); \
  TwoSumSafe(r[2], e, a[2], e); \
  TwoSumSafe(r[3], e, a[3], e); \
  Renorm5(r[0], r[1], r[2], r[3], e); \
}

/* quad-double += double (r += a)*/
#define Add_QD_dIn(a, r) { \
  double e; \
  TwoSumIn(r[0], e, a); \
  TwoSumIn(r[1], e, e); \
  TwoSumIn(r[2], e, e); \
  TwoSumIn(r[3], e, e); \
  Renorm5(r[0], r[1], r[2], r[3], e); \
}

#define ThreeSum(a, b, c) { \
  double t1, t2, t3; \
  TwoSum(t1, t2, a, b); \
  TwoSum(a, t3, c, t1); \
  TwoSum(b, c, t2, t3); \
}

#define ThreeSum2(a, b, c) { \
  double t1, t2, t3; \
  TwoSum(t1, t2, a, b); \
  TwoSum(a, t3, c, t1); \
  b = t2 + t3; \
}

/* quad-double = quad-double * double (s = a * b) */
#define Prod_QD_d(s, a, b) { \
  double p0, p1, p2, p3; \
  double q0, q1, q2; \
  double s4; \
  TwoProd(p0, q0, a[0], b); \
  TwoProd(p1, q1, a[1], b); \
  TwoProd(p2, q2, a[2], b); \
  p3 = a[3] * b; \
  s[0] = p0; \
  TwoSum(s[1], s[2], q0, p1); \
  ThreeSum(s[2], q1, p2); \
  ThreeSum2(q1, q2, p3); \
  s[3] = q1; \
  s4 = q2 + p2; \
  Renorm5(s[0], s[1], s[2], s[3], s4); \
}

/* quad-double = quad-double * double (s = a * b) */
#define Prod_QD_dPart(s, a, b, bh, bl) {  \
    double p0, p1, p2, p3;                      \
    double q0, q1, q2;                          \
    double s4;                                  \
    TwoProdPart(p0, q0, b, bh, bl, a[0]);       \
    TwoProdPart(p1, q1, b, bh, bl, a[1]);       \
    TwoProdPart(p2, q2, b, bh, bl, a[2]);       \
    p3 = a[3] * b;                              \
    s[0] = p0;                                  \
    TwoSum(s[1], s[2], q0, p1);                 \
    ThreeSum(s[2], q1, p2);                     \
    ThreeSum2(q1, q2, p3);                      \
    s[3] = q1;                                  \
    s4 = q2 + p2;                               \
    Renorm5(s[0], s[1], s[2], s[3], s4);        \
}

