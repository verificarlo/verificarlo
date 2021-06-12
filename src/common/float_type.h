#ifndef __FLOAT_TYPE_H_
#define __FLOAT_TYPE_H_

#include <stdint.h>

#ifdef __clang__
/* Define usual vector types */
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));

typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));

typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));

/* Define specialized vector integer types */
typedef int32_t int32_2x __attribute__((ext_vector_type(2)));
typedef int32_t int32_4x __attribute__((ext_vector_type(4)));
typedef int32_t int32_8x __attribute__((ext_vector_type(8)));
typedef int32_t int32_16x __attribute__((ext_vector_type(16)));

typedef uint32_t uint32_2x __attribute__((ext_vector_type(2)));
typedef uint32_t uint32_4x __attribute__((ext_vector_type(4)));
typedef uint32_t uint32_8x __attribute__((ext_vector_type(8)));
typedef uint32_t uint32_16x __attribute__((ext_vector_type(16)));

typedef int64_t int64_2x __attribute__((ext_vector_type(2)));
typedef int64_t int64_4x __attribute__((ext_vector_type(4)));
typedef int64_t int64_8x __attribute__((ext_vector_type(8)));
typedef int64_t int64_16x __attribute__((ext_vector_type(16)));

typedef uint64_t uint64_2x __attribute__((ext_vector_type(2)));
typedef uint64_t uint64_4x __attribute__((ext_vector_type(4)));
typedef uint64_t uint64_8x __attribute__((ext_vector_type(8)));
typedef uint64_t uint64_16x __attribute__((ext_vector_type(16)));
#elif __GNUC__
/* Define usual vector types */
typedef double double2 __attribute__((vector_size(2 * sizeof(double))));
typedef double double4 __attribute__((vector_size(4 * sizeof(double))));
typedef double double8 __attribute__((vector_size(8 * sizeof(double))));
typedef double double16 __attribute__((vector_size(16 * sizeof(double))));

typedef float float2 __attribute__((vector_size(2 * sizeof(float))));
typedef float float4 __attribute__((vector_size(4 * sizeof(float))));
typedef float float8 __attribute__((vector_size(8 * sizeof(float))));
typedef float float16 __attribute__((vector_size(16 * sizeof(float))));

typedef int int2 __attribute__((vector_size(2 * sizeof(int))));
typedef int int4 __attribute__((vector_size(4 * sizeof(int))));
typedef int int8 __attribute__((vector_size(8 * sizeof(int))));
typedef int int16 __attribute__((vector_size(16 * sizeof(int))));

/* Define specialized vector integer types */
typedef int32_t int32_2x __attribute__((vector_size(2 * sizeof(int32_t))));
typedef int32_t int32_4x __attribute__((vector_size(4 * sizeof(int32_t))));
typedef int32_t int32_8x __attribute__((vector_size(8 * sizeof(int32_t))));
typedef int32_t int32_16x __attribute__((vector_size(16 * sizeof(int32_t))));

typedef uint32_t uint32_2x __attribute__((vector_size(2 * sizeof(uint32_t))));
typedef uint32_t uint32_4x __attribute__((vector_size(4 * sizeof(uint32_t))));
typedef uint32_t uint32_8x __attribute__((vector_size(8 * sizeof(uint32_t))));
typedef uint32_t uint32_16x __attribute__((vector_size(16 * sizeof(uint32_t))));

typedef int64_t int64_2x __attribute__((vector_size(2 * sizeof(int64_t))));
typedef int64_t int64_4x __attribute__((vector_size(4 * sizeof(int64_t))));
typedef int64_t int64_8x __attribute__((vector_size(8 * sizeof(int64_t))));
typedef int64_t int64_16x __attribute__((vector_size(16 * sizeof(int64_t))));

typedef uint64_t uint64_2x __attribute__((vector_size(2 * sizeof(uint64_t))));
typedef uint64_t uint64_4x __attribute__((vector_size(4 * sizeof(uint64_t))));
typedef uint64_t uint64_8x __attribute__((vector_size(8 * sizeof(uint64_t))));
typedef uint64_t uint64_16x __attribute__((vector_size(16 * sizeof(uint64_t))));
#else
#error "Compiler must be gcc or clang"
#endif

#endif /* __FLOAT_TYPE_H_ */
