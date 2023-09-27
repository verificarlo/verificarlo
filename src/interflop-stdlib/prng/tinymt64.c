/**
 * @file tinymt64.c
 *
 * @brief 64-bit Tiny Mersenne Twister only 127 bit internal state
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (The University of Tokyo)
 *
 * Copyright (C) 2011 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and The University of Tokyo.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 */
#include "tinymt64.h"

#define MIN_LOOP 8

#if defined(__GNUC__)
/**
 * This function always returns 127
 * @param random not used
 * @return always 127
 */
int tinymt64_get_mexp(tinymt64_t *random __attribute__((unused))) {
  return TINYMT64_MEXP;
}
#else
int tinymt64_get_mexp(tinymt64_t *random) { return TINYMT64_MEXP; }
#endif

/**
 * This function changes internal state of tinymt64.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
void tinymt64_next_state(tinymt64_t *random) {
  uint64_t x;

  random->status[0] &= TINYMT64_MASK;
  x = random->status[0] ^ random->status[1];
  x ^= x << TINYMT64_SH0;
  x ^= x >> 32;
  x ^= x << 32;
  x ^= x << TINYMT64_SH1;
  random->status[0] = random->status[1];
  random->status[1] = x;
  random->status[0] ^= -((int64_t)(x & 1)) & random->mat1;
  random->status[1] ^= -((int64_t)(x & 1)) & (((uint64_t)random->mat2) << 32);
}

/**
 * This function outputs 64-bit unsigned integer from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return 64-bit unsigned pseudorandom number
 */
uint64_t tinymt64_temper(tinymt64_t *random) {
  uint64_t x;
#if defined(LINEARITY_CHECK)
  x = random->status[0] ^ random->status[1];
#else
  x = random->status[0] + random->status[1];
#endif
  x ^= random->status[0] >> TINYMT64_SH8;
  x ^= -((int64_t)(x & 1)) & random->tmat;
  return x;
}

/**
 * This function outputs floating point number from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return floating point number r (1.0 <= r < 2.0)
 */
double tinymt64_temper_conv(tinymt64_t *random) {
  uint64_t x;
  union {
    uint64_t u;
    double d;
  } conv;
#if defined(LINEARITY_CHECK)
  x = random->status[0] ^ random->status[1];
#else
  x = random->status[0] + random->status[1];
#endif
  x ^= random->status[0] >> TINYMT64_SH8;
  conv.u = ((x ^ (-((int64_t)(x & 1)) & random->tmat)) >> 12) |
           UINT64_C(0x3ff0000000000000);
  return conv.d;
}

/**
 * This function outputs floating point number from internal state.
 * Users should not call this function directly.
 * @param random tinymt internal status
 * @return floating point number r (1.0 < r < 2.0)
 */
double tinymt64_temper_conv_open(tinymt64_t *random) {
  uint64_t x;
  union {
    uint64_t u;
    double d;
  } conv;
#if defined(LINEARITY_CHECK)
  x = random->status[0] ^ random->status[1];
#else
  x = random->status[0] + random->status[1];
#endif
  x ^= random->status[0] >> TINYMT64_SH8;
  conv.u = ((x ^ (-((int64_t)(x & 1)) & random->tmat)) >> 12) |
           UINT64_C(0x3ff0000000000001);
  return conv.d;
}

/**
 * This function outputs 64-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 64-bit unsigned integer r (0 <= r < 2^64)
 */
uint64_t tinymt64_generate_uint64(tinymt64_t *random) {
  tinymt64_next_state(random);
  return tinymt64_temper(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using multiplying by (1 / 2^53).
 * @param random tinymt internal status
 * @return floating point number r (0.0 <= r < 1.0)
 */
double tinymt64_generate_double(tinymt64_t *random) {
  tinymt64_next_state(random);
  return (tinymt64_temper(random) >> 11) * TINYMT64_MUL;
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 <= r < 1.0)
 */
double tinymt64_generate_double01(tinymt64_t *random) {
  tinymt64_next_state(random);
  return tinymt64_temper_conv(random) - 1.0;
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (1.0 <= r < 2.0)
 */
double tinymt64_generate_double12(tinymt64_t *random) {
  tinymt64_next_state(random);
  return tinymt64_temper_conv(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 < r <= 1.0)
 */
double tinymt64_generate_doubleOC(tinymt64_t *random) {
  tinymt64_next_state(random);
  return 2.0 - tinymt64_temper_conv(random);
}

/**
 * This function outputs floating point number from internal state.
 * This function is implemented using union trick.
 * @param random tinymt internal status
 * @return floating point number r (0.0 < r < 1.0)
 */
double tinymt64_generate_doubleOO(tinymt64_t *random) {
  tinymt64_next_state(random);
  return tinymt64_temper_conv_open(random) - 1.0;
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param[in] x 64-bit integer
 * @return 64-bit integer
 */
uint64_t ini_func1(uint64_t x) {
  return (x ^ (x >> 59)) * UINT64_C(2173292883993);
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param[in] x 64-bit integer
 * @return 64-bit integer
 */
uint64_t ini_func2(uint64_t x) {
  return (x ^ (x >> 59)) * UINT64_C(58885565329898161);
}

/**
 * This function certificate the period of 2^127-1.
 * @param random tinymt state vector.
 */
void period_certification(tinymt64_t *random) {
  if ((random->status[0] & TINYMT64_MASK) == 0 && random->status[1] == 0) {
    random->status[0] = 'T';
    random->status[1] = 'M';
  }
}

/**
 * This function initializes the internal state array with a 64-bit
 * unsigned integer seed.
 * @param random tinymt state vector.
 * @param seed a 64-bit unsigned integer used as a seed.
 */
void tinymt64_init(tinymt64_t *random, uint64_t seed) {
  int i;
  random->status[0] = seed ^ ((uint64_t)random->mat1 << 32);
  random->status[1] = random->mat2 ^ random->tmat;
  for (i = 1; i < MIN_LOOP; i++) {
    random->status[i & 1] ^= i + UINT64_C(6364136223846793005) *
                                     (random->status[(i - 1) & 1] ^
                                      (random->status[(i - 1) & 1] >> 62));
  }
  period_certification(random);
}

/**
 * This function initializes the internal state array,
 * with an array of 64-bit unsigned integers used as seeds
 * @param random tinymt state vector.
 * @param init_key the array of 64-bit integers, used as a seed.
 * @param key_length the length of init_key.
 */
void tinymt64_init_by_array(tinymt64_t *random, const uint64_t init_key[],
                            int key_length) {
  const int lag = 1;
  const int mid = 1;
  const int size = 4;
  int i, j;
  int count;
  uint64_t r;
  uint64_t st[4];

  st[0] = 0;
  st[1] = random->mat1;
  st[2] = random->mat2;
  st[3] = random->tmat;
  if (key_length + 1 > MIN_LOOP) {
    count = key_length + 1;
  } else {
    count = MIN_LOOP;
  }
  r = ini_func1(st[0] ^ st[mid % size] ^ st[(size - 1) % size]);
  st[mid % size] += r;
  r += key_length;
  st[(mid + lag) % size] += r;
  st[0] = r;
  count--;
  for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
    r = ini_func1(st[i] ^ st[(i + mid) % size] ^ st[(i + size - 1) % size]);
    st[(i + mid) % size] += r;
    r += init_key[j] + i;
    st[(i + mid + lag) % size] += r;
    st[i] = r;
    i = (i + 1) % size;
  }
  for (; j < count; j++) {
    r = ini_func1(st[i] ^ st[(i + mid) % size] ^ st[(i + size - 1) % size]);
    st[(i + mid) % size] += r;
    r += i;
    st[(i + mid + lag) % size] += r;
    st[i] = r;
    i = (i + 1) % size;
  }
  for (j = 0; j < size; j++) {
    r = ini_func2(st[i] + st[(i + mid) % size] + st[(i + size - 1) % size]);
    st[(i + mid) % size] ^= r;
    r -= i;
    st[(i + mid + lag) % size] ^= r;
    st[i] = r;
    i = (i + 1) % size;
  }
  random->status[0] = st[0] ^ st[1];
  random->status[1] = st[2] ^ st[3];
  period_certification(random);
}
