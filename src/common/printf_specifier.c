/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#include <endian.h>
#include <math.h>
#include <printf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "float_const.h"
#include "float_struct.h"
#include "float_utils.h"
#include "generic_builtin.h"

#define STRING_MAX 256

#define BYTE_U32 0xF0000000
#define BYTE_U64 0xF000000000000000
#define GET_BYTE_MASK(X) _Generic((X), uint32_t : BYTE_U32, uint64_t : BYTE_U64)

/* Returns the format string depending on the type of X */
#define GET_BINARY_FMT(X)                                                      \
  _Generic((X), float : "%c%c.%s x 2^%d", double : "%c%c.%s x 2^%ld")

/* Translates an hexadecimal value to the bit representation */
const char *hex_to_bit[16] = {"0000", "0001", "0010", "0011", "0100", "0101",
                              "0110", "0111", "1000", "1001", "1010", "1011",
                              "1100", "1101", "1110", "1111"};

/* Converts an integer to its binary representation */
/* 0xF4 -> 0b11110010                              */
#define UINTN_TO_BIT(X, output)                                                \
  {                                                                            \
    if (X == 0) {                                                              \
      output[0] = '0';                                                         \
      output[1] = '\0';                                                        \
    } else {                                                                   \
      const typeof(X) size_in_bit = sizeof(X) * 8;                             \
      const typeof(X) first_trailing_0 = size_in_bit - FFS(X) + 1;             \
      const typeof(X) ith_byte = GET_BYTE_MASK(X);                             \
      typeof(X) i;                                                             \
      for (i = 0; i < first_trailing_0; i += 4) {                              \
        const typeof(X) byte = (X & ith_byte >> i) >> (size_in_bit - (i + 4)); \
        output[i] = hex_to_bit[byte][0];                                       \
        output[i + 1] = hex_to_bit[byte][1];                                   \
        output[i + 2] = hex_to_bit[byte][2];                                   \
        output[i + 3] = hex_to_bit[byte][3];                                   \
      }                                                                        \
      output[i] = '\0';                                                        \
      while (i--, output[i] != '1') {                                          \
        output[i] = '\0';                                                      \
      }                                                                        \
    }                                                                          \
  }

/* Formats a binaryN to its binary representation */
/* Special case for subnormal numbers */
/* Print a subnormal in the denormalized form */
/* 0.<mantissa> x 2^<exponent> */
/* where <mantissa> is not formatted (with leading 0 kept)  */
/* where <exponent> = {FLOAT,DOUBLE}_MIN_EXP */
#define PRINT_SUBNORMAL_DENORMALIZED(real, s_val)                              \
  {                                                                            \
    implicit_bit = '0';                                                        \
    exponent = real.ieee.exponent - real_exp_comp + 1;                         \
    mantissa = real.ieee.mantissa;                                             \
    UINTN_TO_BIT(mantissa << (real_exp_size + real_sign_size), mantissa_str);  \
    sprintf(s_val, binary_fmt, sign_char, implicit_bit, mantissa_str,          \
            exponent);                                                         \
  }

/* Formats a binaryN to its binary representation */
/* Special case for subnormal numbers */
/* Print a subnormal in the normalized form */
/* 1.<mantissa> x 2^<exponent> */
/* where <mantissa> is formatted (with leading 0 removed)  */
/* where <exponent> = {FLOAT,DOUBLE}_MIN_EXP - (# leadind 0) */
#define PRINT_SUBNORMAL_NORMALIZED(real, s_val)                                \
  {                                                                            \
    implicit_bit = '1';                                                        \
    mantissa = real.ieee.mantissa;                                             \
    mantissa <<= (real_sign_size + real_exp_size);                             \
    offset = CLZ(mantissa) + 1;                                                \
    exponent = -real_exp_min - offset;                                         \
    UINTN_TO_BIT(mantissa << offset, mantissa_str);                            \
    sprintf(s_val, binary_fmt, sign_char, implicit_bit, mantissa_str,          \
            exponent);                                                         \
  }

/* Formats a binaryN to its binary representation */
/* 1.<mantissa> x 2^<exponent>                   */
/* Special cases:                                */
/*  NaN -> +/-nan                                */
/*  Inf -> +/-inf                                */
#define REAL_TO_BINARY(real, s_val, info)                                      \
  {                                                                            \
    const typeof(real.u) real_exp_max = GET_EXP_MAX(real.type);                \
    const typeof(real.u) real_sign_size = GET_SIGN_SIZE(real.type);            \
    const typeof(real.u) real_exp_size = GET_EXP_SIZE(real.type);              \
    const typeof(real.u) real_exp_comp = GET_EXP_COMP(real.type);              \
    const typeof(real.u) real_exp_min = GET_EXP_MIN(real.type);                \
    const typeof(real.type) real_value = real.type;                            \
    const char *binary_fmt = GET_BINARY_FMT(real.type);                        \
    char sign_char = signbit(real_value) ? '-' : '+';                          \
    char mantissa_str[STRING_MAX] = "0";                                       \
    char implicit_bit;                                                         \
    int32_t offset;                                                            \
    typeof(real.u) exponent;                                                   \
    typeof(real.u) mantissa;                                                   \
    const int class = FPCLASSIFY(real_value);                                  \
    switch (class) {                                                           \
    case FP_ZERO:                                                              \
      implicit_bit = '0';                                                      \
      exponent = 0;                                                            \
      sprintf(s_val, binary_fmt, sign_char, implicit_bit, mantissa_str,        \
              exponent);                                                       \
      return;                                                                  \
    case FP_INFINITE:                                                          \
      implicit_bit = '1';                                                      \
      exponent = real_exp_max;                                                 \
      sprintf(s_val, "%cinf", sign_char);                                      \
      return;                                                                  \
    case FP_NAN:                                                               \
      sign_char = '+';                                                         \
      implicit_bit = '1';                                                      \
      exponent = real_exp_max;                                                 \
      sprintf(s_val, "%cnan", sign_char);                                      \
      return;                                                                  \
    case FP_SUBNORMAL:                                                         \
      if (info->alt) {                                                         \
        PRINT_SUBNORMAL_NORMALIZED(real, s_val);                               \
      } else {                                                                 \
        PRINT_SUBNORMAL_DENORMALIZED(real, s_val);                             \
      }                                                                        \
      return;                                                                  \
    case FP_NORMAL:                                                            \
      implicit_bit = '1';                                                      \
      mantissa = real.ieee.mantissa;                                           \
      exponent = real.ieee.exponent - real_exp_comp;                           \
      UINTN_TO_BIT(mantissa << (real_exp_size + real_sign_size),               \
                   mantissa_str);                                              \
      sprintf(s_val, binary_fmt, sign_char, implicit_bit, mantissa_str,        \
              exponent);                                                       \
      return;                                                                  \
    }                                                                          \
  }

/* Wrappers for calling REAL_TO_BINARY on double */
void double_to_binary(double d, char *s_val, const struct printf_info *info) {
  binary64 b64 = {.f64 = d};
  REAL_TO_BINARY(b64, s_val, info);
  return;
}

/* Wrappers for calling REAL_TO_BINARY on float */
void float_to_binary(float f, char *s_val, const struct printf_info *info) {
  binary32 b32 = {.f32 = f};
  REAL_TO_BINARY(b32, s_val, info);
  return;
}

/* Handler for float values */
int bit_float_handler(FILE *stream, const struct printf_info *info,
                      const void *const *args) {
  char output[STRING_MAX] = "0";
  const double *d = (const double *)args[0];
  const float f = (float)*d;
  float_to_binary(f, output, info);
  return fprintf(stream, "%s", output);
}

/* Handler for double values */
int bit_double_handler(FILE *stream, const struct printf_info *info,
                       const void *const *args) {
  char output[STRING_MAX] = "0";
  const double d = *(const double *)args[0];
  double_to_binary(d, output, info);
  return fprintf(stream, "%s", output);
}

/* This hanlder set the right information in the print_info struct */
int bit_handler_arginfo(const struct printf_info *info, size_t n, int *argtypes,
                        int *size) {
  if (n > 0) {
    if (info->is_long) {
      argtypes[0] = PA_DOUBLE;
      size[0] = sizeof(double);
    } else {
      argtypes[0] = PA_FLOAT;
      size[0] = sizeof(float);
    }
  }
  return 1;
}

/* This function checks if the given format is 'b' (float) or 'lb' (double) */
int bit_handler(FILE *stream, const struct printf_info *info,
                const void *const *args) {
  if (info->is_long)
    return bit_double_handler(stream, info, args);
  else
    return bit_float_handler(stream, info, args);
}

/* This function defines the conversion specifier character 'b' */
/* Once registered, %b can be used as %f for floating point values in printf
 * functions */
void register_printf_bit(void) {
  register_printf_specifier('b', bit_handler, bit_handler_arginfo);
}
