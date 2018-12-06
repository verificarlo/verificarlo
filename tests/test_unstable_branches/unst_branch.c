#include <stdio.h>

int main() {

  double a = 0.0;
  for (int i=0; i < 100; i++) a += 0.1;
  for (int i=0; i < 100; i++) a -= 0.1;

  /* The branch below is numerically unstable; depending on the previous
   * rounding errors a is either positive or negative */
  if (a>0.0) {
    printf("positive branch taken\n");
  } else {
    printf("negative branch taken\n");
  }

  return 0;
}
