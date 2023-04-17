#ifndef __QUADMATH_STATS_H__
#define __QUADMATH_STATS_H__

#include "pfp128.h"

/* Computes the means */
FP128 compute_mean(const int N, const FP128 data[]);
/* Knuth compensated online variance */
FP128 compute_variance(const int N, const FP128 mean,
                            const FP128 data[]);
/* Computes the number of significant digits */
double compute_sig(const int N, const FP128 data[]);

#endif /* __QUADMATH_STATS_H__ */
