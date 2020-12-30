#include <stdio.h>
#include <stdlib.h>

// Type definition
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));

#define perform_vector_binary_op(size, op, res, a, b)			\
  switch (size) {							\
  case 2:								\
  case 4:								\
  case 8:								\
  case 16:								\
    switch (op) {							\
    case '+':								\
      res = a + b;							\
      break;								\
    case '*':								\
      res = a * b;							\
      break;								\
    case '-':								\
      res = a - b;							\
      break;								\
    case '/':								\
      res = a / b;							\
      break;								\
    default:								\
      fprintf(stderr, "Bad op %c\n", op);				\
      exit(EXIT_FAILURE);						\
      break;								\
    };									\
    break;								\
  default:								\
    fprintf(stderr, "Bad size %llu\n", size);				\
    exit(EXIT_FAILURE);							\
    break;								\
  };

int main(int argc, char **argv) {

  if (argc < 6) {
    fprintf(stderr, "at least 6 arguments expected: type op size a b\n");
    exit(EXIT_FAILURE);
  }

  char *precision = argv[1];
  char op = argv[2][0];
  unsigned long long size = strtoll(argv[3], NULL, 10);

  REAL a = strtod(argv[4], NULL);
  REAL b = strtod(argv[5], NULL);
  REAL res = 0.0;
  
  printf("%s %c %lld\n", precision, op, size);
  
  perform_vector_binary_op(size, op, res, a, b);

  for (unsigned long long i = 0; i < size; ++i) {
    printf("%lf\n", res[i]);
  }

  return 0;
}
