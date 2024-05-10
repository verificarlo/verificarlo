
/*********************** XOROSHIRO128++ FUNCTIONS ****************************\
 *                                                                           *\
 *  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)   *\
 *                                                                           *\
 *  To the extent possible under law, the author has dedicated all copyright *\
 *  and related and neighboring rights to this software to the public domain *\
 *  worldwide. This software is distributed without any warranty.            *\
 *                                                                           *\
 *  See <http://creativecommons.org/publicdomain/zero/1.0/>.                 *\
 *                                                                           *\
 *                                                                           *\
 *  This is xoroshiro128++ 1.0, one of our all-purpose, rock-solid,          *\
 *  small-state generators. It is extremely (sub-ns) fast and it passes all  *\
 *  tests we are aware of, but its state space is large enough only for      *\
 *  mild parallelism.                                                        *\
 *                                                                           *\
 *****************************************************************************/

#include <stdint.h>

#include "splitmix64.h"
#include "xoroshiro128.h"

static inline uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

uint64_t next(xoroshiro_state s) {
  const uint64_t s0 = s[0];
  uint64_t s1 = s[1];
  const uint64_t result = rotl(s0 + s1, 17) + s0;

  s1 ^= s0;
  s[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21); // a, b
  s[1] = rotl(s1, 28);                   // c

  return result;
}

/*
  Taken from https://prng.di.unimi.it/
  "The code above cooks up by bit manipulation a real number in the interval
  [1..2), and then subtracts one to obtain a real number in the interval
  [0..1). If x is chosen uniformly among 64-bit integers, d is chosen uniformly
  among dyadic rationals of the form k / 2−52. This is the same technique used
  by generators providing directly doubles, such as the dSFMT."
*/
static inline double to_double(uint64_t x) {
  const union {
    uint64_t i;
    double d;
  } u = {.i = UINT64_C(0x3FF) << 52 | x >> 12};
  return u.d - 1.0;
}

double next_double(xoroshiro_state s) { return to_double(next(s)); }
