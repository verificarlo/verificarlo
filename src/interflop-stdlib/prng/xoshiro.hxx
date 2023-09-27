#ifndef __PRNG_XOSHIRO_HXX__
#define __PRNG_XOSHIRO_HXX__

#include <stdint.h>

typedef uint64_t xoshiro128_state_t[2];
typedef uint64_t xoshiro256_state_t[4];

uint32_t xoshiro128plus_next(xoshiro128_state_t &s);
uint32_t xoshiro128plusplus_next(xoshiro128_state_t &s);
uint32_t xoshiro128starstar_next(xoshiro128_state_t &s);
uint64_t xoroshiro128plus_next(xoshiro128_state_t &s);
uint64_t xoroshiro128plusplus_next(xoshiro128_state_t &s);
uint64_t xoroshiro128starstar_next(xoshiro128_state_t &s);
uint64_t xoshiro256plus_next(xoshiro256_state_t &s);
uint64_t xoshiro256plusplus_next(xoshiro256_state_t &s);
uint64_t xoshiro256starstar_next(xoshiro256_state_t &s);
uint64_t splitmix64_next(uint64_t &x);
void init_xoshiro256_state(xoshiro256_state_t &state, uint64_t seed);
void init_xoshiro128_state(xoshiro128_state_t &state, uint64_t seed);
float xoshiro_uint32_to_float(const uint32_t i);
double xoshiro_uint64_to_double(const uint64_t i);

#endif /* __PRNG_XOSHIRO_HXX__ */