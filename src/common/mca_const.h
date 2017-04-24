//Round to nearest using cast
//Works for standard type, aka double to float, if the ieee rounding flag is set to nearest
//WARNING: For quad to double we notice that the behavior is always round toward zero
#define NEAREST_FLOAT(x)	((float) (x))
#define	NEAREST_DOUBLE(x)	((double) (x))


//Quad precision exponent encoding size
#define QUAD_EXP_SIZE      15
//Quad precision pseudo mantissa encoding size 
#define QUAD_PMAN_SIZE  112
//Quad precision pseudo mantissa encoding size in the word containing the 64 msb
#define QUAD_HX_PMAN_SIZE  48
//Quad precison pseudo mantissa encoding size in the word containing the 64 lsb
#define QUAD_LX_PMAN_SIZE  64
//Quad precison mantissa size
#define QUAD_PREC          113
//Quad precison exponent complement
#define QUAD_EXP_COMP      16383
//Quad precison max exponent
#define QUAD_EXP_MAX       16383
//Quad precison min exponent
#define QUAD_EXP_MIN       16382
//Quad precison mask to remove the sign bit
#define QUAD_HX_ERASE_SIGN 0x7fffffffffffffffULL
//Quad precison 64 msb to encode plus infinity
#define QINF_hx            0x7fff000000000000ULL
//Quad precison 64 msb to encode minus infinity
#define QMINF_hx            0x7fff000000000000ULL
//Quad precison 64 lsb to encode plus infinity
#define QINF_lx            0x0000000000000000ULL
//Quad precison 64 lsb to encode minus infinity
#define QMINF_lx            0x0000000000000000ULL
//Quad precision pseudo mantissa msb set to one
#define QUAD_HX_PMAN_MSB   0x0000800000000000ULL

//Double precision expoenent encoding size
#define DOUBLE_EXP_SIZE    11
//Double precision pseudo-mantissa encoding size
#define DOUBLE_PMAN_SIZE   52
//Double precision mantissa size 
#define DOUBLE_PREC        53
//Double precison exponent complement
#define DOUBLE_EXP_COMP    1023
//Double precison max exponent
#define DOUBLE_EXP_MAX     1023
//Double precison min exponent
#define DOUBLE_EXP_MIN     1022
//Double precision plus infinity encoding
#define DOUBLE_PLUS_INF    0x7FF0000000000000ULL
//Double precision pseudo matissa msb set to one
#define DOUBLE_PMAN_MSB    0x0008000000000000ULL
//Double precision mask to erase sign bit
#define DOUBLE_ERASE_SIGN  0x7fffffffffffffffULL
//Double precision mask to extract sign bit
#define DOUBLE_GET_SIGN    0x8000000000000000ULL
//Double precision mask to extract the pseudo mantissa
#define DOUBLE_GET_PMAN    0x000fffffffffffffULL

//Single precision exponent encoding size
#define FLOAT_EXP_SIZE     8
//Single precision pseudo mantisa encoding size
#define FLOAT_PMAN_SIZE    23
//single precision mantissa size
#define FLOAT_PREC         24

//Sign encoding size
#define SIGN_SIZE          1
//64bit word with msb set to 1
#define WORD64_MSB         0x8000000000000000ULL


