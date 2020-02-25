#ifndef __QUADMATH_STATS_H__
#define __QUADMATH_STATS_H__

/* Computes the means */
__float128 compute_mean(const int N, const REAL data[]);
/* Knuth compensated online variance */
__float128 compute_variance(const int N, const __float128 mean,
                            const REAL data[]);
/* Computes the number of significant digits */
double compute_sig(const int N, const REAL data[]);

#endif /* __QUADMATH_STATS_H__ */
