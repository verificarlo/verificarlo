#ifndef __BITMASK_CONST_H__
#define __BITMASK_CONST_H__

#define foutput "bitmask_ref"

#ifdef DOUBLE
 #define REAL double
 #ifndef INV
   #define REAL_TEST_VALUE 0x801fffffffffffff  // -0x1.fffffffffffffp+1022
 #else 
   #define REAL_TEST_VALUE 0x4000000000000000  // 0x1p+1
 #endif
 #define xfmt "%a"
 #define REAL_MASK_1 0xFFFFFFFFFFFFFFFFULL
 #define MANTISSA_SIZE 53
 #define INT_T uint64_t
 #define MINUS_ONE (-1LL)
 #define NEAREST_REAL(x) ((double) (x))
#else
 #define REAL float
  #ifndef INV
   #define REAL_TEST_VALUE 0x80ffffff // -0x1.fffffep-126
  #else
   #define REAL_TEST_VALUE 0x40000000 // 0x1p+1
 #endif
 #define xfmt "%a"
 #define REAL_MASK_1 0xFFFFFFFFULL
 #define MANTISSA_SIZE 24
 #define INT_T uint32_t
 #define MINUS_ONE (-1)
 #define NEAREST_REAL(x) ((float) (x))
#endif

#ifndef INV
 #define APPLY_MASK(x,m) (x) &= (m)
#else
 #define APPLY_MASK(x,m) (x) |= ~(m)
#endif

#endif /* __BITMASK_CONST_H__ */
