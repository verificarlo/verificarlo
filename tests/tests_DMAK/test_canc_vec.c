#include <stdio.h>


int main(int argc, char **argv)
{
  int a[12], b[12];  
  a[0] =0x3FFFFFFF;
  b[0] =0x3FFFFFFE;
  a[1] =0x3FFFFFFF;
  b[1] =0x3FFFFFFC;
  a[2] =0x3FFFFFFF;
  b[2] =0x3FFFFFF8;
  a[3] =0x3FFFFFFF;
  b[3] =0x3FFFFFF0;
  a[4] =0x3FFFFFFF;
  b[4] =0x3FFFFFE0;
  a[5] =0x3FFFFFFF;
  b[5] =0x3FFFFFC0;
  a[6] =0x3FFFFFFF;
  b[6] =0x3FFFFF80;
  a[7] =0x3FFFFFFF;
  b[7] =0x3FFFFF00;
  a[8] =0x3FFFFFFF;
  b[8] =0x3FFFFE00;
  a[9] =0x3FFFFFFF;
  b[9] =0x3FFFFC00;
  a[10]=0x3FFFFFFF;
  b[10]=0x3FFFF800;
  a[11]=0x3FFFFFFF;
  b[11]=0x3FFFF000;

  float A[12], B[12], C[12];
  int i = 0;
  for(i = 0; i < 12; i++)
    {
      A[i] = *((float*) &a[i]);
      B[i] = *((float*) &b[i]);
      C[i] = A[i] - B[i];
      printf("   A[%d]   = %f \n", i+1, A[i]);
      printf("   B[%d]   = %f \n", i+1, B[i]);
      printf("A[%d]-B[%d] = %f \n\n", i+1, i+1, C[i]);
    }
  return 0;
}
