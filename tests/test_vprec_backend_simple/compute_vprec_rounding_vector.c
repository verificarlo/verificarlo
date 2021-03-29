#include <stdio.h>
#include <stdlib.h>

typedef float float2 __attribute__((ext_vector_type(2)));

float2 perform_bin_op(float2 a, float2 b, char op) {
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

  float2 a = {strtod(argv[1], NULL), strtod(argv[3], NULL)};
  float2 b = {strtod(argv[2], NULL), strtod(argv[4], NULL)};
  char op = argv[5][0];

  float2 c = perform_bin_op(a, b, op);

  for (int i = 0; i < 2; i++)
    printf("%a\n", c[i]);

  return 0;
}
