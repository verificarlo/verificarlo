#ifndef __INTERFLOP_FMA_H__
#define __INTERFLOP_FMA_H__

#include <bits/floatn.h>

#if defined(__cplusplus)
extern "C" {
#endif

float interflop_fma_binary32(float a, float b, float c);
double interflop_fma_binary64(double a, double b, double c);
_Float128 interflop_fma_binary128(_Float128 a, _Float128 b, _Float128 c);

#if defined(__cplusplus)
}
#endif

#endif /* __INTERFLOP_FMA_H__ */