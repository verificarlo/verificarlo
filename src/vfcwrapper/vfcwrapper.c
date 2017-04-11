#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "vfcwrapper.h"


#define MAX_BACKENDS 16

struct numdbg_backend_interface_t backends[MAX_BACKENDS];
void * contexts[MAX_BACKENDS];
unsigned char active_backends = 0;

extern struct numdbg_backend_interface_t numdbg_init(void ** context); 

/* vfc_init is run when loading vfcwrapper and initializes vfc backends */
__attribute__((constructor))
static void vfc_init (void)
{

  /* FIXME: Register each backend in turn, for now only a single backend is registered */

  /* FIXME: Instead of linking against the backend library, a better option
     is to load the backend(s) as plugins with dlopen() and call
     for each plugin the function numdbg_init() followed by register_backend() */

  if (active_backends == MAX_BACKENDS) {
    fprintf(stderr, "No more than %d backends can be used simultaneously", MAX_BACKENDS);
  }
  backends[active_backends] = numdbg_init(&contexts[active_backends]);
  active_backends ++;
}


typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef bool bool2 __attribute__((ext_vector_type(2)));
typedef bool bool4 __attribute__((ext_vector_type(4)));

/* Arithmetic wrappers */

float _floatadd(float a , float b) {
  float c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_add_float(a, b, &c, NULL);
  }
  return c;
}

float _floatsub(float a, float b) {
  float c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_sub_float(a, b, &c, NULL);
  }
  return c;
}

float _floatmul(float a, float b) {
  float c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_mul_float(a, b, &c, NULL);
  }
  return c;
}

float _floatdiv(float a, float b) {
  float c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_div_float(a, b, &c, NULL);
  }
  return c;
}


double _doubleadd(double a , double b) {
  double c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_add_double(a, b, &c, NULL);
  }
  return c;
}

double _doublesub(double a, double b) {
  double c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_sub_double(a, b, &c, NULL);
  }
  return c;
}

double _doublemul(double a, double b) {
  double c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_mul_double(a, b, &c, NULL);
  }
  return c;
}

double _doublediv(double a, double b) {
  double c;
  for (int i = 0; i < active_backends; i++) {
    backends[i].numdbg_div_double(a, b, &c, NULL);
  }
  return c;
}

/* Arithmetic vector wrappers */

double2 _2xdoubleadd(double2 a, double2 b) {
    double2 c;

    c[0] = _doubleadd(a[0],b[0]);
    c[1] = _doubleadd(a[1],b[1]);
    return c;
}

double2 _2xdoublesub(double2 a, double2 b) {
    double2 c;

    c[0] = _doublesub(a[0],b[0]);
    c[1] = _doublesub(a[1],b[1]);
    return c;
}

double2 _2xdoublemul(double2 a, double2 b) {
    double2 c;

    c[0] = _doublemul(a[0],b[0]);
    c[1] = _doublemul(a[1],b[1]);
    return c;
}

double2 _2xdoublediv(double2 a, double2 b) {
    double2 c;

    c[0] = _doublediv(a[0],b[0]);
    c[1] = _doublediv(a[1],b[1]);
    return c;
}


/*********************************************************/

double4 _4xdoubleadd(double4 a, double4 b) {
    double4 c;

    c[0] = _doubleadd(a[0],b[0]);
    c[1] = _doubleadd(a[1],b[1]);
    c[2] = _doubleadd(a[2],b[2]);
    c[3] = _doubleadd(a[3],b[3]);
    return c;
}

double4 _4xdoublesub(double4 a, double4 b) {
    double4 c;

    c[0] = _doublesub(a[0],b[0]);
    c[1] = _doublesub(a[1],b[1]);
    c[2] = _doublesub(a[2],b[2]);
    c[3] = _doublesub(a[3],b[3]);
    return c;
}

double4 _4xdoublemul(double4 a, double4 b) {
    double4 c;

    c[0] = _doublemul(a[0],b[0]);
    c[1] = _doublemul(a[1],b[1]);
    c[2] = _doublemul(a[2],b[2]);
    c[3] = _doublemul(a[3],b[3]);
    return c;
}

double4 _4xdoublediv(double4 a, double4 b) {
    double4 c;

    c[0] = _doublediv(a[0],b[0]);
    c[1] = _doublediv(a[1],b[1]);
    c[2] = _doublediv(a[2],b[2]);
    c[3] = _doublediv(a[3],b[3]);
    return c;
}


/*********************************************************/


float2 _2xfloatadd(float2 a, float2 b) {
    float2 c;

    c[0] = _floatadd(a[0],b[0]);
    c[1] = _floatadd(a[1],b[1]);
    return c;
}

float2 _2xfloatsub(float2 a, float2 b) {
    float2 c;

    c[0] = _floatsub(a[0],b[0]);
    c[1] = _floatsub(a[1],b[1]);
    return c;
}

float2 _2xfloatmul(float2 a, float2 b) {
    float2 c;

    c[0] = _floatmul(a[0],b[0]);
    c[1] = _floatmul(a[1],b[1]);
    return c;
}

float2 _2xfloatdiv(float2 a, float2 b) {
    float2 c;

    c[0] = _floatdiv(a[0],b[0]);
    c[1] = _floatdiv(a[1],b[1]);
    return c;
}

float4 _4xfloatadd(float4 a, float4 b) {
    float4 c;

    c[0] = _floatadd(a[0],b[0]);
    c[1] = _floatadd(a[1],b[1]);
    c[2] = _floatadd(a[2],b[2]);
    c[3] = _floatadd(a[3],b[3]);
    return c;
}

float4 _4xfloatsub(float4 a, float4 b) {
    float4 c;

    c[0] = _floatsub(a[0],b[0]);
    c[1] = _floatsub(a[1],b[1]);
    c[2] = _floatsub(a[2],b[2]);
    c[3] = _floatsub(a[3],b[3]);
    return c;
}

float4 _4xfloatmul(float4 a, float4 b) {
    float4 c;

    c[0] = _floatmul(a[0],b[0]);
    c[1] = _floatmul(a[1],b[1]);
    c[2] = _floatmul(a[2],b[2]);
    c[3] = _floatmul(a[3],b[3]);
    return c;
}

float4 _4xfloatdiv(float4 a, float4 b) {
    float4 c;

    c[0] = _floatdiv(a[0],b[0]);
    c[1] = _floatdiv(a[1],b[1]);
    c[2] = _floatdiv(a[2],b[2]);
    c[3] = _floatdiv(a[3],b[3]);
    return c;
}

