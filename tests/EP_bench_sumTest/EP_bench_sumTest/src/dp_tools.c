//#######################################
//## Author: Eric Petit
//## adapted from Arnaud Tisserand C code
//## Mail eric.petit@prism.uvsq.fr
//#######################################

#include "dp_tools.h"

void _dp_error_(char *file, int l, char *s) {
  fprintf(stderr, "%s: %d, %s", file, l, s);
  exit(EXIT_FAILURE);
}

/*********************** __ARCH_x86__ *******************************/
void dp_init(int s) {
  #if defined(__ARCH_x86__) || defined(__ARCH_x86_64__)
  static volatile fpu_control_t oldcw;
  volatile fpu_control_t newcw;
  #endif
  #if defined(__ARCH_ia64__)
  static volatile unsigned long long int oldfpsr;
  #endif
  
  if( s == DP_RNDN ) {
    #if defined(__ARCH_x86__) || defined(__ARCH_x86_64__)
      _FPU_GETCW(oldcw);
      newcw = 0x027f;
      _FPU_SETCW(newcw);
    #elif defined(__ARCH_ia64__) && defined(__GCC__)
      __asm__ __volatile__("mov %0=ar.fpsr;;" : "=r"(oldfpsr) :: "memory");
      __asm__("fsetc.s0 0,8\n");
    #elif defined(__ARCH_ia64__) && defined(__ICC__)
      oldfpsr = __getReg(_IA64_REG_AR_FPSR);
      _Asm_fsetc(0x00,0x08,0);
    #endif
  }
  else if( s == DP_REST ) {
    #if defined(__ARCH_x86__) || defined(__ARCH_x86_64__)
      _FPU_SETCW(oldcw);
    #elif defined(__ARCH_ia64__) && defined(__GCC__)
      __asm__ __volatile__("mov ar.fpsr=%0;;" :: "r"(oldfpsr));
    #elif defined(__ARCH_ia64__) && defined(__ICC__)
      __setReg(_IA64_REG_AR_FPSR, oldfpsr);
    #endif
  }
  else dp_error("DPInit");
}

/********************************************************************/

double rand_double(double M) {
  return((M * (double)rand()) / (double)RAND_MAX);
}

double rand_unif(double a, double b) {
  return(a+(b-a)*rand()/RAND_MAX);
}

unsigned int rand_uint(unsigned int M) {
  return((unsigned int)(((M+1.0)*rand()) / (RAND_MAX+1.0)));
}