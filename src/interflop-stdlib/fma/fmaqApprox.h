#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

__float128 fmaqApprox(__float128 x, __float128 y, __float128 z);

double fmadApprox(double x, double y, double z);

float fmafApprox(float x, float y, float z);

#if defined(__cplusplus)
}
#endif