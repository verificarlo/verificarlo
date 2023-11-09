#ifndef RAND_H
#define RAND_H

#include <stdint.h>

uint64_t get_rand_uint64(void);
int32_t get_rand_float(float);
int64_t get_rand_double(double);

#endif // RAND_H