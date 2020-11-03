#include <stdio.h>
#include <stdlib.h>

REAL perform_bin_op(REAL a, REAL b, char op) {
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

  if (argc != 4) {
    fprintf(stderr, "3 arguments expected: a b op\n");
    exit(EXIT_FAILURE);
  }

  REAL a = strtod(argv[1], NULL);
  REAL b = strtod(argv[2], NULL);
  char op = argv[3][0];

  REAL c = perform_bin_op(a,b,op);

  printf("%a\n",c);

  return 0;
}
