#pragma once
#include <stdint.h>

/* =========================================================================
 * Vector type definitions (GCC/Clang vector extensions)
 * ========================================================================= */

typedef float float2 __attribute__((vector_size(2 * sizeof(float))));
typedef float float4 __attribute__((vector_size(4 * sizeof(float))));
typedef float float8 __attribute__((vector_size(8 * sizeof(float))));
typedef float float16 __attribute__((vector_size(16 * sizeof(float))));

typedef double double2 __attribute__((vector_size(2 * sizeof(double))));
typedef double double4 __attribute__((vector_size(4 * sizeof(double))));
typedef double double8 __attribute__((vector_size(8 * sizeof(double))));
typedef double double16 __attribute__((vector_size(16 * sizeof(double))));

typedef int32_t int2 __attribute__((vector_size(2 * sizeof(int32_t))));
typedef int32_t int4 __attribute__((vector_size(4 * sizeof(int32_t))));
typedef int32_t int8 __attribute__((vector_size(8 * sizeof(int32_t))));
typedef int32_t int16 __attribute__((vector_size(16 * sizeof(int32_t))));
