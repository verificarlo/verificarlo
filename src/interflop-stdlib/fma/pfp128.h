//===-- pfp128.h - Architecture and compiler neutral shims for 128b IEEE FP
//--------------*- C -*-===//
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/*
 * A portability shim header to provide support for IEEE FP128
 * on machines/compilers which have the underlying support, but don't
 * provide the C standard interfaces.
 */
// Header monotonicity.
#if (!defined(_PFP128_H_INCLUDED_))
#define _PFP128_H_INCLUDED_ 1

// Tell the system headers that we want all of the types, please.
// (Not that it seems to do much good!)
#define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#include <complex.h>
#include <float.h>
#include <math.h>

// Architecture neutral C standard stuff, which we hope will catch what is going
// on! The compiler supports the IEC 559 specification, however, that does not
// actually require support for the 128b type. So we check whether the property

// macros for FLT128 are present

// Zeroed out for now, since although GCC sets this on x86_64, linux,
// the foof128 function names are not present in libm :-(
#if (0 && ((defined(__STDC_IEC_60559_TYPES__) || defined(__STDC_IEC_559__)) && \
           defined(FLT128_MAX)))
#if (PFP128_SHOW_CONFIG)
#warning __STDC_IEC_60559_TYPES__ or __STDC_IEC_559__ defined with FLT128_MAX
#endif
typedef _Float128 FP128;
typedef _Complex _Float128 COMPLEX_FP128;
// No format specifier is given in the standard...
// We assume for now that we're dealing with a GNU implementation and that 'Q'
// will work.
#define FP128_FMT_TAG "Q"
// No way to do this with a static inline because the function takes an ellipsis
// argument list, and there is no version which takes a va_list.
#define FP128_snprintf quadmath_snprintf
#define FP128_CONST(val) val##F128
#define FP128Name(function) function##f128
static inline FP128 strtoFP128(char const *s, char **sp) {
  return strtoflt128(s, sp);
}
#if (PFP128_SHOW_CONFIG)
#warning __LONG_DOUBLE_IEEE128__ defined
#warning FP128 is _Float128
#warning FP128_CONST tag is F128
#warning FP128 function suffix is f128
#warning strtoFP128 => strtof128
#warning 
#endif
#elif (defined(__LONG_DOUBLE_IEEE128__))
// No standard conformant support has been promised by the implementation.
// So we have to guess based on the target and compiler.
// First check for the case we believe does not provide any support.
#define FP128_IS_LONGDOUBLE 1
#elif (__APPLE__ && __MACH__ && __aarch64__)
#warning No IEEE 128b float seems to be available on MacOS/AArch64...
#warning FP128 will only be the same as double (i.e. 64b).
#if (PFP128_SHOW_CONFIG)
#warning Invoked with PFP128_SHOW_CONFIG: targeting MacOS AArch64
#endif
// Use long double, but it'll only be 64b
#define FP128_IS_LONGDOUBLE 1
#elif (__arm__)
#warning No IEEE 128b float seems to be available on 32b Arm...
#warning FP128 will only be the same as double (i.e. 64b).
// Use long double, but it'll only be 64b, so this name is a bit misleading
// but it has the appropriate effect.
#define FP128_IS_LONGDOUBLE 1
#elif (__aarch64__)
// We can use long double with both LLVM and GCC,
// and, other than on MacOS, that gets us what we want.
#define FP128_IS_LONGDOUBLE 1
#if (PFP128_SHOW_CONFIG)
#warning Invoked with PFP128_SHOW_CONFIG: targeting AArch64 (not MacOS)
#endif
#elif (__x86_64__)
#if (PFP128_SHOW_CONFIG)
#warning Invoked with PFP128_SHOW_CONFIG: targeting x86_64
#endif

// long double is not the IEEE 128b format and we need libquadmath
#include <quadmath.h>

// Need to step very carefully around 80b floats!
typedef __float128 FP128; // Both LLVM and GCC support the __float128 type,
typedef __complex128 COMPLEX_FP128;

#define FP128_CONST(val) val##Q
#define FP128Name(function) function##q
#define FP128_FMT_TAG "Q"
// No way to do this with a static inline because the function takes an ellipsis
// argument list, and there is no version which takes a va_list.
#define FP128_snprintf quadmath_snprintf

static inline FP128 strtoFP128(char const *s, char **sp) {
  return strtoflt128(s, sp);
}

#if (PFP128_SHOW_CONFIG)
#warning FP128 is __float128
#warning FP128_CONST tag is Q
#warning FP128 function suffix is q
#warning FP128_FMT_TAG is Q
#warning FP128_snprintf => quadmath_snprintf
#warning strtoFP128 => strtoflt128
#endif

#else
#error On an architecture this code does not understand.
#endif

#if (FP128_IS_LONGDOUBLE)
// The compiler supports IEEE 128b float as long double.
// Or we are pretending that it does...
// The C standard only requires that long double
// is at least as large as double, so it  may be accepted
// syntactically, but not get you any more precision.
typedef long double FP128; // Both LLVM and GCC support long double
typedef _Complex long double COMPLEX_FP128;

#define FP128_CONST(val) val##L
#define FP128Name(function) function##l
#define FP128_FMT_TAG "L"
#define FP128_snprintf snprintf

#if (PFP128_SHOW_CONFIG)
#warning FP128 is long double
#warning FP128_CONST tag is L
#warning FP128 function suffix is l
#warning FP128_FMT_TAG is L
#warning FP128_snprintf => snprintf
#warning strtoFP128 => strtold
#endif

#include <stdlib.h>
static inline FP128 strtoFP128(char const *s, char **sp) {
  return strtold(s, sp);
}
#endif

// From here on the code is common no matter what the underlying implementation.
// So if you're adding more functions do it here.

// Maths functions
// Implememt inline static shims.

// clang-format really messes up the multiple line macro definitions :-(
// clang-format off
#define CreateUnaryShim(basename, restype, argtype)	\
static inline restype basename ## FP128(argtype arg) {	\
  return FP128Name(basename)(arg);			\
}

// Could do all of this with one macro if we passed in the argument
// list rather than the argument types, but it's not much simpler.
#define CreateBinaryShim(basename, restype, at1, at2)           \
static inline restype basename ## FP128(at1 arg1, at2 arg2) {	\
  return FP128Name(basename)(arg1, arg2);			\
}

#define CreateTernaryShim(basename, restype, at1, at2, at3)             \
static inline restype basename ## FP128(at1 arg1, at2 arg2, at3 arg3) { \
  return FP128Name(basename)(arg1, arg2, arg3);				\
}

#define FOREACH_UNARY_FUNCTION(op)              \
  op(acos, FP128, FP128)                        \
  op(acosh, FP128, FP128)                       \
  op(asin, FP128, FP128)                        \
  op(asinh, FP128, FP128)                       \
  op(atan, FP128, FP128)                        \
  op(atanh, FP128, FP128)                       \
  op(cbrt, FP128, FP128)                        \
  op(ceil, FP128, FP128)                        \
  op(cosh, FP128, FP128)                        \
  op(cos, FP128, FP128)                         \
  op(erf, FP128, FP128)                         \
  op(erfc, FP128, FP128)                        \
  op(exp, FP128, FP128)                         \
  op(expm1, FP128, FP128)                       \
  op(fabs, FP128, FP128)                        \
  op(floor, FP128, FP128)                       \
  op(ilogb, int, FP128)                         \
  op(lgamma, FP128, FP128)                      \
  op(llrint, long long int, FP128)              \
  op(llround, long long int, FP128)             \
  op(logb, FP128, FP128)                        \
  op(log, FP128, FP128)                         \
  op(log10, FP128, FP128)                       \
  op(log2, FP128, FP128)                        \
  op(log1p, FP128, FP128)                       \
  op(lrint, long int, FP128)                    \
  op(lround, long int, FP128)                   \
  op(nan, FP128, const char *)                  \
  op(nearbyint, FP128, FP128)                   \
  op(rint, FP128, FP128)                        \
  op(round, FP128, FP128)                       \
  op(sinh, FP128, FP128)                        \
  op(sin, FP128, FP128)                         \
  op(sqrt, FP128, FP128)                        \
  op(tan, FP128, FP128)                         \
  op(tanh, FP128, FP128)                        \
  op(tgamma, FP128, FP128)                      \
  op(trunc, FP128, FP128)                       \
  op(cabs, FP128, COMPLEX_FP128)                \
  op(carg, FP128, COMPLEX_FP128)                \
  op(cimag, FP128, COMPLEX_FP128)               \
  op(creal, FP128, COMPLEX_FP128)               \
  op(cacos, COMPLEX_FP128, COMPLEX_FP128)       \
  op(cacosh, COMPLEX_FP128, COMPLEX_FP128)      \
  op(casin, COMPLEX_FP128, COMPLEX_FP128)       \
  op(casinh, COMPLEX_FP128, COMPLEX_FP128)      \
  op(catan, COMPLEX_FP128, COMPLEX_FP128)       \
  op(catanh, COMPLEX_FP128, COMPLEX_FP128)      \
  op(ccos, COMPLEX_FP128, COMPLEX_FP128)        \
  op(ccosh, COMPLEX_FP128, COMPLEX_FP128)       \
  op(cexp, COMPLEX_FP128, COMPLEX_FP128)        \
  op(clog, COMPLEX_FP128, COMPLEX_FP128)        \
  op(conj, COMPLEX_FP128, COMPLEX_FP128)        \
  op(cproj, COMPLEX_FP128, COMPLEX_FP128)       \
  op(csin, COMPLEX_FP128, COMPLEX_FP128)        \
  op(csinh, COMPLEX_FP128, COMPLEX_FP128)       \
  op(csqrt, COMPLEX_FP128, COMPLEX_FP128)       \
  op(ctan, COMPLEX_FP128, COMPLEX_FP128)        \
  op(ctanh, COMPLEX_FP128, COMPLEX_FP128)

// exp2 doesn't seem to be available everywhere.
// If you need it, and have it, then add it in the obvious way.
// op(exp2, FP128, FP128)			

FOREACH_UNARY_FUNCTION(CreateUnaryShim)

// Functions with two arguments
#define FOREACH_BINARY_FUNCTION(op)                     \
  op(ldexp, FP128, FP128, int)                          \
  op(modf, FP128, FP128, FP128 *)                       \
  op(nextafter, FP128, FP128, FP128)                    \
  op(pow, FP128, FP128, FP128)                          \
  op(remainder, FP128, FP128, FP128)                    \
  op(cpow, COMPLEX_FP128, COMPLEX_FP128, COMPLEX_FP128) \
  op(atan2, FP128, FP128, FP128)                        \
  op(copysign, FP128, FP128, FP128)                     \
  op(fdim, FP128, FP128, FP128)                         \
  op(fmax, FP128, FP128, FP128)                         \
  op(fmin, FP128, FP128, FP128)                         \
  op(fmod, FP128, FP128, FP128)                         \
  op(frexp, FP128, FP128, int *)                        \
  op(hypot, FP128, FP128, FP128)

FOREACH_BINARY_FUNCTION(CreateBinaryShim)

// Functions with three arguments
#define FOREACH_TERNARY_FUNCTION(op)            \
  op(remquo, FP128, FP128, FP128, int *)        \
  op(fma, FP128, FP128, FP128, FP128)

FOREACH_TERNARY_FUNCTION(CreateTernaryShim)

// clang-format on

// Constants
// I have no idea why there is the inconsistency in naming between these
// constants which have the type at the front, and the others which have the
// type tag at the end but that's how the standard headers work, so we follow.

// Properties of the type
// Since the whole point is that this header is giving us IEEE 128b float we can
// put the actual values in here and not bother to delegate.
#define FP128_MAX FP128_CONST(1.18973149535723176508575932662800702e4932)
#define FP128_MIN FP128_CONST(3.36210314311209350626267781732175260e-4932)
#define FP128_EPSILON FP128_CONST(1.92592994438723585305597794258492732e-34)
#define FP128_DENORM_MIN                                                       \
  FP128_CONST(6.475175119438025110924438958227646552e-4966)
#define FP128_MANT_DIG 113
#define FP128_MIN_EXP (-16381)
#define FP128_MAX_EXP 16384
#define FP128_DIG 33
#define FP128_MIN_10_EXP (-4931)
#define FP128_MAX_10_EXP 4932
#define HUGE_VALFP128 FP128Name(HUGE_VAL)

// Properties of mathematics; we delegate here just for simplicity.
#define M_E_FP128 FP128_CONST(2.718281828459045235360287471352662498) /* e */
#define M_LOG2E_FP128                                                          \
  FP128_CONST(1.442695040888963407359924681001892137) /* log_2 e */
#define M_LOG10E_FP128                                                         \
  FP128_CONST(0.434294481903251827651128918916605082) /* log_10 e */
#define M_LN2_FP128                                                            \
  FP128_CONST(0.693147180559945309417232121458176568) /* log_e 2 */
#define M_LN10_FP128                                                           \
  FP128_CONST(2.302585092994045684017991454684364208) /* log_e 10 */
#define M_PI_FP128 FP128_CONST(3.141592653589793238462643383279502884) /* pi   \
                                                                        */
#define M_PI_2_FP128                                                           \
  FP128_CONST(1.570796326794896619231321691639751442) /* pi/2 */
#define M_PI_4_FP128                                                           \
  FP128_CONST(0.785398163397448309615660845819875721) /* pi/4 */
#define M_1_PI_FP128                                                           \
  FP128_CONST(0.318309886183790671537767526745028724) /* 1/pi */
#define M_2_PI_FP128                                                           \
  FP128_CONST(0.636619772367581343075535053490057448) /* 2/pi */
#define M_2_SQRTPI_FP128                                                       \
  FP128_CONST(1.128379167095512573896158903121545172) /* 2/sqrt(pi) */
#define M_SQRT2_FP128                                                          \
  FP128_CONST(1.414213562373095048801688724209698079) /* sqrt(2) */
#define M_SQRT1_2_FP128                                                        \
  FP128_CONST(0.707106781186547524400844362104849039) /* 1/sqrt(2) */

// Cleanliness
#undef FP128_IS_LONGDOUBLE
#undef FP128Name
#undef FOREACH_UNARY_FUNCTION
#undef FOREACH_BINARY_FUNCTION
#undef FOREACH_TERNARY_FUNCTION
#undef CreateUnaryShim
#undef CreateBinaryShim
#undef CreateTernaryShim

#endif // Header monotonicity
