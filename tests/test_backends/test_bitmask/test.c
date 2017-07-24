#include <stdio.h>
#include <stdint.h>

int main(int argc, char *argv[]){
  int a = 1;
  float b = 2.0;
  uint32_t *t = (void*)&a;
  printf("%016f %08a %08x\n",*((float*)t),*((float*)t),*((unsigned int*)t));
  t = (void*)&b;
  printf("%016f %08a %08x\n",*((float*)t),*((float*)t),*((unsigned int*)t));
  (*t) = (*t) | 0x00800000;
  printf("%016f %08a %08x\n",*((float*)t),*((float*)t),*((unsigned int*)t));
  (*t) = (*t) | 0x008000aa;
  printf("%016f %08a %08x %016e\n",*((float*)t),*((float*)t),*((unsigned int*)t),*((float*)t));
  return 0;
}
