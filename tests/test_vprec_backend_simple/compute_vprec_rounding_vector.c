#include <stdio.h>
#include <stdlib.h>

typedef REAL REAL2 __attribute__((ext_vector_type(2)));

REAL2 perform_bin_op(REAL2 a, REAL2 b, char op) {
  switch(op){
  case '+':
    return (a) + (b);
  case '-':
    return (a) - (b);
  case 'x':
    return (a) * (b);
  case '/':
    return (a) / (b);
  default:
    fprintf(stderr, "Bad op %c\n",op);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char * argv[]) {

  if (argc != 6) {
    fprintf(stderr, "5 arguments expected: a1 b1 a2 b2 op\n");
    exit(EXIT_FAILURE);
  }

  REAL2 a = {strtod(argv[1], NULL), strtod(argv[3], NULL)};
  REAL2 b = {strtod(argv[2], NULL), strtod(argv[4], NULL)};
  char op = argv[5][0];

  REAL2 c = perform_bin_op(a, b, op);

  for (int i = 0; i < 2; i++)
    printf("%a\n", c[i]);

  return 0;
}
