//#######################################
//## Author: Arnaud Tisserand
//#######################################

#ifndef __DP_TOOLS__
#define __DP_TOOLS__

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* Standard constantes in IEEE-754 double precision */
/* Machine epsilon = 2^(-52) */
#define EPS_DBL 2.220446049250313080847263336181640625000000000000000000000000000e-16
/* Unit roundoff in rounding to the nearest rounding mode = 2^(-53) */
#define URD_DBL 1.110223024625156540423631668090820312500000000000000000000000000e-16
/* Half the unit roundoff in rounding to the nearest rounding mode = 2^(-54) */
#define HALF_URD_DBL 0.5551115123125782702118158340454101562500000000000000000000e-16
/* Smallest positive normalized floating point number = 2^(-1022) */
#define LBD_DBL 2.225073858507201383090232717332404064219215980462331830553327417e-308
/* Underflow unit = 2^(-1074) */
#define ETA_DBL 4.9406564584124654417656879286822137236505980261432476442558568251e-324


#ifndef ABS
#  define ABS(x) ((x)>=0.0?(x):-(x))
#endif

#ifndef SGN
#  define SGN(x) ((x)>=0.0?1.0:-1.0)
#endif

#ifndef gam
#  define gam(n) (((n)*URD_DBL)/(1-(n)*URD_DBL))
#endif

#define DP_RNDN      1
#define DP_REST      2

void _dp_error_(char *file, int l, char *s);
#define dp_error(s) _dp_error_(__FILE__, __LINE__, s)
void dp_init(int s);

/* ----------------------------------------------------------------- */
/* --- Architecture dependend macros and constants ----------------- */
/* ----------------------------------------------------------------- */

#if defined(__ARCH_x86__) || defined(__x86_64__)

  #include <fpu_control.h>
  #ifndef _FPU_SETCW
    #define _FPU_SETCW(cw) __asm__ ("fldcw %0" : : "m" (*&cw))
  #endif
  #ifndef _FPU_GETCW
    #define _FPU_GETCW(cw) __asm__ ("fnstcw %0" : "=m" (*&cw))
  #endif

#elif defined(__ARCH_ia64__) && defined(__GCC__)

  #define __ARCH_HAS_FMA__
  /* It is not necessary to add a ';;' after the fma.d instruction, because gcc add it automatically. */
  #define FMA(r, a, b, c) __asm__("fma.d %0 = %1, %2, %3\n" : "=f"(r) : "f"(a), "f"(b), "f"(c) );
  #define FMS(r, a, b, c) __asm__("fms.d %0 = %1, %2, %3\n" : "=f"(r) : "f"(a), "f"(b), "f"(c) );

#elif defined(__ARCH_ia64__) && defined(__ICC__)

  #define __ARCH_HAS_FMA__
  #include <ia64intrin.h>
  #include <ia64regs.h>
  /* How to access the Floating Point Status Register with ICC:
  FPSR is defined in ia64regs.h as "_IA_64_REG_AR_FPSR 3112" */
  #ifndef _IA64_REG_AR_FPSR
    #define _IA64_REG_AR_FPSR 3112
  #endif
  #define FMA(r, a, b, c) r = (a)*(b)+(c)
  #define FMS(r, a, b, c) r = (a)*(b)-(c)
  /* Here _PC_D = 2 and _SF0 = 0*/
  /*#define FMA(r, a, b, c)  r = _Asm_fma(2, a, b, c, 0)*/
  /*#define FMS(r, a, b, c)  r = _Asm_fms(2, a, b, c, 0)*/

#endif

/* ----------------------------------------------------------------- */
/* --- TwoSum ------------------------------------------------------ */
/* ----------------------------------------------------------------- */

#if defined(__ARCH_ia64__) && defined(__GCC__) && defined(__OPT__)

  /* GCC seems to generate appropriate predicated
     instructions with this code */
  #define TwoSuma(s, e, a, b) { \
    double min_, max_; \
    s = a+b; \
    max_ = (fabs(a) > fabs(b) ? a : b); \
    min_ = (fabs(a) > fabs(b) ? b : a); \
    e = (max_ - s) + min_; \
  }

  /* GCC seems to generate   */
  #define TwoSumIna(s, e, b) {          \
    double min_, max_ , t_; \
    t_ = s+b; \
    max_ = (fabs(s) > fabs(b) ? s : b); \
    min_ = (fabs(s) > fabs(b) ? b : s); \
    e = (max_ - t_) + min_; \
    s = t_; \
  }

  /* Cf Cornea et al., Scientif Computing on Itanium-based Systems, p291 */
  #define TwoSum(s, e, a, b) { \
    double min_, max_; \
    __asm__("famax %0 = %1, %2\n" : "=f"(max_) : "f"(a), "f"(b) ); \
    __asm__("famin %0 = %1, %2\n" : "=f"(min_) : "f"(b), "f"(a) ); \
    s = a+b; \
    e = (max_ - s) + min_; \
  }

  #define TwoSumIn(s, e, b) { \
    double min_, max_ , t_; \
    __asm__("famax %0 = %1, %2\n" : "=f"(max_) : "f"(s), "f"(b) ); \
    __asm__("famin %0 = %1, %2\n" : "=f"(min_) : "f"(b), "f"(s) ); \
    t_ = s+b; \
    e = (max_ - t_) + min_; \
    s = t_; \
  }

#elif defined(__ARCH_ia64__) && defined(__ICC__) && defined(__OPT__)

  /* Cf Cornea et al., Scientif Computing on Itanium-based Systems, p291 */
  /*
  #define TwoSum(a, b, s, e) { \
    double min_, max_, t_; \
    s = a + b; \
    min_ = _Asm_famin(a, b); \
    max_ = _Asm_famax(b, a); \
    t_ = max_ - s; \
    e = t_ + min_; \
  }

  #define TwoSumIn(s, e, b) { \
    double min_, max_, t_; \
    t0 = a + b; \
    min_ = _Asm_famin(s, b); \
    max_ = _Asm_famax(b, s); \
    t1_ = max_ - t0_; \
    e = t1_ + min_; \
    s = t0_; \
  }
  */

  /* (s, e) = TwoSum(a, b) */
  #define TwoSum(s, e, a, b) { \
  double t0_, t1_, t2_, t3_;\
    s = (a) + b; \
    t0_ = s - (a); \
    t1_ = s - t0_; \
    t2_ = (b) - t0_; \
    t3_ = (a) - t1_; \
    e = t2_ + t3_; \
  }

  /* (s, e) = TwoSumIn(s, b) */
  #define TwoSumIn(s, e, b) { \
  double s_, t0_, t1_, t2_, t3_;  \
    s_ = (s) + b; \
    t0_ = s_ - (s); \
    t1_ = s_ - t0_; \
    t2_ = (b) - t0_; \
    t3_ = (s) - t1_; \
    e = t2_ + t3_; \
    s = s_; \
  }

#else

  /* (s, e) = TwoSum(a, b) */
  #define TwoSum(s, e, a, b) { \
  double t0_, t1_, t2_, t3_;\
    s = (a) + b; \
    t0_ = s - (a); \
    t1_ = s - t0_; \
    t2_ = (b) - t0_; \
    t3_ = (a) - t1_; \
    e = t2_ + t3_; \
  }

  /* (s, e) = TwoSumIn(s, b) */
  #define TwoSumIn(s, e, b) { \
  double s_, t0_, t1_, t2_, t3_;  \
    s_ = (s) + b; \
    t0_ = s_ - (s); \
    t1_ = s_ - t0_; \
    t2_ = (b) - t0_; \
    t3_ = (s) - t1_; \
    e = t2_ + t3_; \
    s = s_; \
  }

#endif

/* We assume here that |a| >= |b| */
/* (s, e) = FastTwoSum(a, b) */
#define FastTwoSum(s, e, a, b) { \
  double s_, t_; \
  s_ = a + b; \
  t_ = a - s_; \
  e = t_ + b; \
  s = s_; \
}

#define NextPowerTwo(res, n) ({\
  double _q = n / URD_DBL; \
  if((res = fabs((_q+n)-_q)) == 0.0) res = fabs(n);\
})

/* ----------------------------------------------------------------- */
/* --- TwoProd ----------------------------------------------------- */
/* ----------------------------------------------------------------- */

#if !defined(__ARCH_HAS_FMA__)

  /*2^27+1*/
  #define _splitter_ 134217729.0

  /* (p, e) = TwoProd(a, b) */
  #define TwoProd(p, e, a, b) {            \
    double ashift_, bshift_;               \
    double ahi_, alo_;                     \
    double bhi_, blo_;                     \
    double t1_, t2_;                       \
    ashift_ = (a) * _splitter_;  bshift_ = (b) * _splitter_; \
    t1_ = ashift_ - (a);         t2_ = bshift_ - (b);        \
    ahi_ = ashift_ - t1_;        bhi_ = bshift_ - t2_;       \
    alo_ = a - ahi_;             blo_ = b - bhi_;            \
    p = (a) * (b);                                           \
    e = (((ahi_*bhi_-p)+ahi_*blo_)+alo_*bhi_)+alo_*blo_; }

  #define Split(ahi, alo, a) {  \
    double ashift_, t_;         \
    ashift_ = (a) * _splitter_; \
    t_ = ashift_ - (a);         \
    ahi = ashift_ - t_;         \
    alo = a - ahi; }


  /* (p, e) = TwoProd(a, b) */
  #define TwoProdPart(p, e, a, ahi, alo, b) { \
    double bshift_, bhi_, blo_, t_;           \
    bshift_ = (b) * _splitter_;               \
    t_ = bshift_ - (b);                       \
    bhi_ = bshift_ - t_;                      \
    blo_ = b - bhi_;                          \
    p = (a) * (b);                            \
    e = (((ahi*bhi_-p)+ahi*blo_)+ alo * bhi_)+alo*blo_; }

#else /* defined(__ARCH_HAS_FMA__) */

  /* (p, e) = TwoProd(a, b) */
  #define TwoProd(p, e, a, b) { p = a * b; FMS(e, a, b, p); }

  /* (r, r2, r3) = ThreeFMA(a, x, y) */
  #define ThreeFMA(r, r2, r3, a, x, y) {\
    double r1, t1, t2, u1, u2, a1, b1, b2; \
    FMA(r1, a, x, y); u1 = a * x; \
    FMS(u2, a, x, u1); \
    a1 = y + u2; \
    t1 = a1 - y;                       b1 = u1 + a1; \
    r3 = (y - (a1 - t1)) + (u2 - t1);  t2 = b1 - u1;\
                                       b2 = (u1 - (b1 - t2)) + (a1 - t2); \
    r2 = (b1 - r1) + b2; \
    r = r1; }

#endif

/* ----------------------------------------------------------------- */
/* --- TwoDiv ------------------------------------------------------ */
/* ----------------------------------------------------------------- */

#if !defined(__ARCH_HAS_FMA__)

  /*2^27+1*/
  #define _splitter_ 134217729.0

  /* (q, r) = DivRem(a, b) */
  #define DivRem(q, r, a, b) { \
    double x_, y_;             \
    double qshift_, bshift_;   \
    double qhi_, qlo_;         \
    double bhi_, blo_;         \
    double t1_, t2_;           \
    q = (a) / (b);             \
    qshift_ = (q) * _splitter_;  bshift_ = (b) * _splitter_; \
    t1_ = qshift_ - (q);         t2_ = bshift_ - (b);        \
    qhi_ = qshift_ - t1_;        bhi_ = bshift_ - t2_;       \
    qlo_ = q - qhi_;             blo_ = b - bhi_;            \
    x_ = (q) * (b);                                          \
    y_ = (((qhi_*bhi_-x_)+qhi_*blo_)+ qlo_*bhi_)+ qlo_*blo_;  \
    r = (a - x_) - y_; }

  /* (q, e) = ApproxTwoDiv(a, b) */
  #define ApproxTwoDiv(q, e, a, b) { \
    double x_, y_;             \
    double qshift_, bshift_;   \
    double qhi_, qlo_;         \
    double bhi_, blo_;         \
    double t1_, t2_;           \
    q = (a) / (b);             \
    qshift_ = (q) * _splitter_;  bshift_ = (b) * _splitter_; \
    t1_ = qshift_ - (q);         t2_ = bshift_ - (b);        \
    qhi_ = qshift_ - t1_;        bhi_ = bshift_ - t2_;       \
    qlo_ = q - qhi_;             blo_ = b - bhi_;            \
    x_ = (q) * (b);                                          \
    y_ = (((qhi_*bhi_-x_)+qhi_*blo_)+ qlo_*bhi_)+ qlo_*blo_;  \
    e = (a - x_ - y_) / (b); }

#else /* defined(__ARCH_HAS_FMA__) */

  #define _splitter_ 134217729.0

  /* (q, r) = DivRem(a, b) */
  #define DivRem(q, r, a, b) { \
    double x_, y_;             \
    q = (a) / (b);             \
    x_ = (q) * (b);            \
    FMS(y_, q, b, x_);         \
    r = (a - x_) - y_; }

  /* (q, e) = DivRem(a, b) */
  #define ApproxTwoDiv(q, e, a, b) { \
    double x_, y_;             \
    q = (a) / (b);             \
    x_ = (q) * (b);            \
    FMS(y_, q, b, x_);         \
    e = ((a - x_) - y_) / (b); }

#endif

double rand_double(double M);
double rand_unif(double a, double b);
unsigned int rand_uint(unsigned int M);

#endif /*__DP_TOOLS__*/