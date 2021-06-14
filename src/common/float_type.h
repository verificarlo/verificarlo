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

#endif

#endif /* __FLOAT_TYPE_H_ */
