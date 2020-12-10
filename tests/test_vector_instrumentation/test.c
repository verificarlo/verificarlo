
// Type definition
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));

REAL f_add(REAL a, REAL b) {
  return (a + b);
}

REAL f_mul(REAL a, REAL b) {
  return (a * b);
}

REAL f_sub(REAL a, REAL b) {
  return (a - b);
}

REAL f_div(REAL a, REAL b) {
  return (a / b);
}
