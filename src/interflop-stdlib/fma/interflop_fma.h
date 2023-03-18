#ifndef __INTERFLOP_FMA_H__
#define __INTERFLOP_FMA_H__

#if defined(__cplusplus)
extern "C" {
#endif

float interflop_fma_binary32(float a, float b, float c);
double interflop_fma_binary64(double a, double b, double c);
__float128 interflop_fma_binary128(__float128 a, __float128 b, __float128 c);

#if defined(__cplusplus)
}
#endif

#endif /* __INTERFLOP_FMA_H__ */