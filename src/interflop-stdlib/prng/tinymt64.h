#ifndef TINYMT64_H
#define TINYMT64_H
/**
 * @file tinymt64.h
 *
 * @brief Tiny Mersenne Twister only 127 bit internal state
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

#include <inttypes.h>
#include <stdint.h>

#define TINYMT64_MEXP 127
#define TINYMT64_SH0 12
#define TINYMT64_SH1 11
#define TINYMT64_SH8 8
#define TINYMT64_MASK UINT64_C(0x7fffffffffffffff)
#define TINYMT64_MUL (1.0 / 9007199254740992.0)

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * tinymt64 internal state vector and parameters
 */
struct TINYMT64_T {
  uint64_t status[2];
  uint32_t mat1;
  uint32_t mat2;
  uint64_t tmat;
};

typedef struct TINYMT64_T tinymt64_t;
int tinymt64_get_mexp(tinymt64_t *random __attribute__((unused)));
void tinymt64_next_state(tinymt64_t *random);
uint64_t tinymt64_temper(tinymt64_t *random);
double tinymt64_temper_conv(tinymt64_t *random);
double tinymt64_temper_conv_open(tinymt64_t *random);
uint64_t tinymt64_generate_uint64(tinymt64_t *random);
double tinymt64_generate_double(tinymt64_t *random);
double tinymt64_generate_double01(tinymt64_t *random);
double tinymt64_generate_double12(tinymt64_t *random);
double tinymt64_generate_doubleOC(tinymt64_t *random);
double tinymt64_generate_doubleOO(tinymt64_t *random);
uint64_t ini_func1(uint64_t x);
uint64_t ini_func2(uint64_t x);
void period_certification(tinymt64_t *random);
void tinymt64_init(tinymt64_t *random, uint64_t seed);
void tinymt64_init_by_array(tinymt64_t *random, const uint64_t init_key[],
                            int key_length);

#if defined(__cplusplus)
}
#endif

#endif
