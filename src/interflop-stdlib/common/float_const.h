#ifndef __FLOAT_CONST_H__
#define __FLOAT_CONST_H__

// Round to nearest using cast
// Works for standard type, aka double to float, if the ieee rounding flag is
// set to nearest WARNING: For quad to double we notice that the behavior is
// always round toward zero
#define NEAREST_FLOAT(x) ((float)(x))
#define NEAREST_DOUBLE(x) ((double)(x))

// Quad precision sign encoding size
#define QUAD_SIGN_SIZE 1
// Quad precision quiet nan bit encoding size
#define QUAD_QUIET_NAN_SIZE 1
// Quad precision exponent encoding size
#define QUAD_EXP_SIZE 15
// Quad precision pseudo mantissa encoding size
#define QUAD_PMAN_SIZE 112
// Quad precision pseudo mantissa encoding size in the word containing the 64
// msb
#define QUAD_HX_PMAN_SIZE 48
// Quad precison pseudo mantissa encoding size in the word containing the 64 lsb
#define QUAD_LX_PMAN_SIZE 64
// Quad precision pseudo mantissa encoding size for quiet nan in the word
// containing the 64 msb
#define QUAD_HX_PMAN_QNAN_SIZE 47
// Quad precison pseudo mantissa encoding size for quiet nan in the word
// containing the 64 lsb
#define QUAD_LX_PMAN_QNAN_SIZE 64
// Quad precison mantissa size
#define QUAD_PREC 113
// Quad precison exponent complement
#define QUAD_EXP_COMP 16383
// Quad precison max exponent
#define QUAD_EXP_MAX 16383
// Quad precison min exponent
#define QUAD_EXP_MIN 16382
// Quad precison infinite exponent
#define QUAD_EXP_INF 0x7FFF
// Quad precison mask to remove the sign bit
#define QUAD_HX_ERASE_SIGN 0x7fffffffffffffffULL
// Quad precison 64 msb to encode plus infinity
#define QINF_hx 0x7fff000000000000ULL
// Quad precison 64 msb to encode minus infinity
#define QMINF_hx 0x7fff000000000000ULL
// Quad precison 64 lsb to encode plus infinity
#define QINF_lx 0x0000000000000000ULL
// Quad precison 64 lsb to encode minus infinity
#define QMINF_lx 0x0000000000000000ULL
// Quad precision pseudo mantissa msb set to one
#define QUAD_HX_PMAN_MSB 0x0000800000000000ULL

// Double precision encoding size
#define DOUBLE_SIGN_SIZE 1
// Double precision expoenent encoding size
#define DOUBLE_EXP_SIZE 11
// Double precision pseudo-mantissa encoding size
#define DOUBLE_PMAN_SIZE 52
// Double precision mantissa size
#define DOUBLE_PREC 53
// Double precison exponent complement
#define DOUBLE_EXP_COMP 1023
// Double precison max exponent for normal number
#define DOUBLE_NORMAL_EXP_MAX 1023
// Double precison max exponent
#define DOUBLE_EXP_MAX 1024
// Double precison min exponent
#define DOUBLE_EXP_MIN 1022
// Double precison infinite exponent
#define DOUBLE_EXP_INF 0x7FF
// Double precision plus infinity encoding
#define DOUBLE_PLUS_INF 0x7FF0000000000000ULL
// Double precision pseudo matissa msb set to one
#define DOUBLE_PMAN_MSB 0x0008000000000000ULL
// Double precision mask to erase sign bit
#define DOUBLE_ERASE_SIGN 0x7fffffffffffffffULL
// Double precision mask to extract sign bit
#define DOUBLE_GET_SIGN 0x8000000000000000ULL
// Double precision mask to extract the exponent
#define DOUBLE_GET_EXP 0x7ff0000000000000ULL
// Double precision mask to extract the pseudo mantissa
#define DOUBLE_GET_PMAN 0x000fffffffffffffULL
// Double precision high part mantissa
#define DOUBLE_PMAN_HIGH_SIZE 20
// Double precision low part mantissa
#define DOUBLE_PMAN_LOW_SIZE 32
// Double precision mask of 1
#define DOUBLE_MASK_ONE 0xffffffffffffffffULL

// single precision encoding size
#define FLOAT_SIGN_SIZE 1
// Single precision exponent encoding size
#define FLOAT_EXP_SIZE 8
// Single precision pseudo mantisa encoding size
#define FLOAT_PMAN_SIZE 23
// single precision mantissa size
#define FLOAT_PREC 24
// single precison exponent complement
#define FLOAT_EXP_COMP 127
// single precison max exponent for normal number
#define FLOAT_NORMAL_EXP_MAX 127
// single precison max exponent
#define FLOAT_EXP_MAX 128
// single precison min exponent
#define FLOAT_EXP_MIN 126
// single precison infinite exponent
#define FLOAT_EXP_INF 0xFF
// single precision plus infinity encoding
#define FLOAT_PLUS_INF 0x7F800000
// Single precision pseudo matissa msb set to one
#define FLOAT_PMAN_MSB 0x00400000
// Single precision mask to erase sign bit
#define FLOAT_ERASE_SIGN 0x7fffffff
// Single precision mask to extract sign bit
#define FLOAT_GET_SIGN 0x80000000
// Single precision mask to extract the exponent
#define FLOAT_GET_EXP 0x7F800000
// Single precision mask to extract the pseudo mantissa
#define FLOAT_GET_PMAN 0x007fffff
// Single precision mask of 1
#define FLOAT_MASK_ONE 0xffffffffULL

// Sign encoding size
#define SIGN_SIZE 1
// 64bit word with msb set to 1
#define WORD64_MSB 0x8000000000000000ULL

#endif /* __FLOAT_CONST_H__ */
