#include <stdlib.h>
#include <stdio.h>

#define xstr(s) str(s)
#define str(s) #s
#define concat(x) loop_##x
#define xconcat(x) concat(x)

#define define_loop(N)				\
  float xconcat(N)(float *A, int n) {	\
    float s = 0;				\
    for (int i = 0; i < n; i++) {		\
      s += A[i];				\
    }						\
    return s;					\
  }

#define loop(A, n) xconcat(REAL)(A,n)
define_loop(REAL);

int main(int argc, char * argv[]) {

  int n = atoi(argv[1]);

  float *A = malloc(sizeof(float)*n);
  
  printf("%s %f\n", xstr(N), loop(A, n));

  return 0;
}
