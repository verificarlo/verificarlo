#include <stdlib.h>
#include <stdio.h>

#include <vfctracer-probe.h>

struct real {
  float x;
  double y;
  float t[10];
} real_t;

float f(float a, float b) {
  return a + b;
}

int main(int argc, char *argv[]) {

  int n = atoi(argv[1]);
  float a = atof(argv[2]);
  float b = atof(argv[3]);

  float c = f(a,b);
  
  float sum = 0.0;

  char name_x[] = "r->x"; 
  
  struct real *r = (struct real*)malloc(sizeof(struct real));
  
  for (int i = 0; i < n; i++) {
    sum += c*0.1;
    vfc_probe_binary32(&sum,"sum");
  }

  r->x = 10;
  r->t[5] = 42;
  
  vfc_probe_binary64(&(r->y),"r->y");
  vfc_probe_binary32(&r->x,name_x);
  vfc_probe_binary32(&r->t[5],"r->t[5]");

  return 0;
}
