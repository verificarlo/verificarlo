/************************** SPLITMIX64 FUNCTION *******************************\
 *                                                                            *\
 *  Written in 2015 by Sebastiano Vigna (vigna@acm.org)                       *\
 *                                                                            *\
 *  To the extent possible under law, the author has dedicated all copyright  *\
 *  and related and neighboring rights to this software to the public domain  *\
 *  worldwide. This software is distributed without any warranty.             *\
 *                                                                            *\
 *  See <http://creativecommons.org/publicdomain/zero/1.0/>.                  *\
 *                                                                            *\
 *                                                                            *\
 *  This is a fixed-increment version of Java 8's SplittableRandom generator  *\
 *  See http://dx.doi.org/10.1145/2714064.2660195 and                         *\
 *  http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html  *\
 *                                                                            *\
 *  It is a very fast generator passing BigCrush, and it can be useful if     *\
 *  for some reason you absolutely want 64 bits of state.                     *\
 *                                                                            *\
 ******************************************************************************/

#include <stdint.h>

uint64_t next_seed(uint64_t seed_state) {
  uint64_t z = (seed_state += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}
