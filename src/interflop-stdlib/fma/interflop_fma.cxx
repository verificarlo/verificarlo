#include "interflop_fma.h"
#include "fmaqApprox.h"

#if HAVE_FMA_INTRINSIC
#include "vr_fma.hxx"
#define __interflop_fma_b32(a, b, c) vr_fma(a, b, c)
#define __interflop_fma_b64(a, b, c) vr_fma(a, b, c)
#define __interflop_fma_b128(a, b, c) fmaqApprox(a, b, c)
#else
#define __interflop_fma_b32(a, b, c) fmafApprox(a, b, c)
#define __interflop_fma_b64(a, b, c) fmadApprox(a, b, c)
#define __interflop_fma_b128(a, b, c) fmaqApprox(a, b, c)
#endif

float interflop_fma_binary32(float a, float b, float c) {
  return __interflop_fma_b32(a, b, c);
}

double interflop_fma_binary64(double a, double b, double c) {
  return __interflop_fma_b64(a, b, c);
}

__float128 interflop_fma_binary128(__float128 a, __float128 b, __float128 c) {
  return __interflop_fma_b128(a, b, c);
}