#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#include "vfcwrapper.h"


#define MAX_BACKENDS 16

struct numdbg_backend_interface_t backends[MAX_BACKENDS];
void * contexts[MAX_BACKENDS];
unsigned char loaded_backends = 0;
unsigned char active_backends = 0;

typedef struct numdbg_backend_interface_t (*numdbg_init_t)(void ** context);

/* vfc_set_active_backends changes how many backends are active,
   at start all the loaded backends are active.

   If vfc_set_active_backends in called with 0 <= n <= loaded_backends,
   then only the first n-th backends will be used.
 */
void vfc_set_active_backends(int n)
{
  if (n <= 0) {
    errx(1, "vfc_set_active_backends(%d): at least one backend must remain active",
         n);
  }

  if (n > loaded_backends) {
    errx(1, "vfc_set_active_backends(%d): cannot activate more backend than loaded",
         n);
  }

  active_backends = n;
}

/* vfc_init is run when loading vfcwrapper and initializes vfc backends */
__attribute__((constructor))
static void vfc_init (void)
{
  /* Parse VFC_BACKENDS */
  char * vfc_backends = getenv("VFC_BACKENDS");
  if (vfc_backends == NULL) {
    errx(1, "VFC_BACKENDS is empty, at least one backend should be provided");
  }

  /* For each backend, load and register the backend vtable interface */
  char* token = strtok(vfc_backends, " ");
  while (token) {
    warn("Loading backend %s", token);

    /* load the backend .so */
    void * handle = dlopen(token, RTLD_NOW);
    if (handle == NULL) {
      errx(1, "Cannot load backend %s: %s", token, strerror(errno));
    }

    /* reset dl errors */
    dlerror();

    /* get the address of the numdbg_init function */
    numdbg_init_t handle_init = (numdbg_init_t) dlsym(handle, "numdbg_init");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
      errx(1, "Cannot find numdbg_init function in backend %s: %s", token,
           strerror(errno));
    }

    /* Register backend */
    if (loaded_backends == MAX_BACKENDS) {
      fprintf(stderr, "No more than %d backends can be used simultaneously",
              MAX_BACKENDS);
    }
    backends[loaded_backends] = handle_init(&contexts[active_backends]);
    loaded_backends ++;


    /* parse next backend token */
    token = strtok(NULL, "");
  }

  /* At start all the loaded backends are called */
  active_backends = loaded_backends;
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

