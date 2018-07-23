/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2015                                                          *
 *     Universite de Versailles St-Quentin-en-Yvelines                          *
 *     CMLA, Ecole Normale Superieure de Cachan                                 *
 *                                                                              *
 *  Verificarlo is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by        *
 *  the Free Software Foundation, either version 3 of the License, or           *
 *  (at your option) any later version.                                         *
 *                                                                              *
 *  Verificarlo is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 *  GNU General Public License for more details.                                *
 *                                                                              *
 *  You should have received a copy of the GNU General Public License           *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.        *
 *                                                                              *
 ********************************************************************************/

#include <errno.h>
#include <err.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>

#include "vfcwrapper.h"

#include "libmca-mpfr.h"
#include "libmca-quad.h"
#include "libbitmask.h"

#define VERIFICARLO_PRECISION "VERIFICARLO_PRECISION"
#define VERIFICARLO_MCAMODE "VERIFICARLO_MCAMODE"
#define VERIFICARLO_BACKEND "VERIFICARLO_BACKEND"
#define VERIFICARLO_BITMASK_MODE "VERIFICARLO_BITMASK_MODE"
#define VERIFICARLO_PRECISION_DEFAULT 53
#define VERIFICARLO_MCAMODE_DEFAULT MCAMODE_MCA
#define VERIFICARLO_BACKEND_DEFAULT MCABACKEND_MPFR
#define VERIFICARLO_BITMASK_MODE_DEFAULT BITMASK_MODE_ZERO
#define SIZE_MAX_BACKTRACE 128

/* Set default values for MCA*/
int verificarlo_precision = VERIFICARLO_PRECISION_DEFAULT;
int verificarlo_mcamode = VERIFICARLO_MCAMODE_DEFAULT;
int verificarlo_backend = VERIFICARLO_BACKEND_DEFAULT;
int verificarlo_bitmask_mode = VERIFICARLO_BITMASK_MODE_DEFAULT;

/* File for outputting values produced by veritracer */
static FILE *trace_FILE_ptr = NULL;
static FILE *backtrace_FILE_ptr = NULL;
static int backtrace_fd = -1;
static void *backtrace_buffer[SIZE_MAX_BACKTRACE];
static char string_buffer[SIZE_MAX_BACKTRACE];
static const char trace_filename[] = "veritracer.dat";
static const char backtrace_filename[] = "backtrace.dat";
static const char backtrace_separator[] = "###\n";

/* This is the vtable for the current MCA backend */
struct mca_interface_t _vfc_current_mca_interface;

/* Activates the mpfr MCA backend */
static void vfc_select_interface_mpfr(void) {
    _vfc_current_mca_interface = mpfr_mca_interface;
    _vfc_current_mca_interface.set_mca_precision(verificarlo_precision);
    _vfc_current_mca_interface.set_mca_mode(verificarlo_mcamode);
}

/* Activates the quad MCA backend */
static void vfc_select_interface_quad(void) {
    _vfc_current_mca_interface = quad_mca_interface;
    _vfc_current_mca_interface.set_mca_precision(verificarlo_precision);
    _vfc_current_mca_interface.set_mca_mode(verificarlo_mcamode);
}

/* Activates the bitmask backend */
static void vfc_select_interface_bitmask(void) {
    _vfc_current_mca_interface = bitmask_interface;
    _vfc_current_mca_interface.set_mca_precision(verificarlo_precision);
    _vfc_current_mca_interface.set_mca_mode(verificarlo_bitmask_mode);
}

/* seeds all the MCA backends */
void vfc_seed(void) {
    mpfr_mca_interface.seed();
    quad_mca_interface.seed();
    bitmask_interface.seed();
}

void vfc_tracer(void) {
  trace_FILE_ptr = fopen(trace_filename ,"wb");
  if (trace_FILE_ptr == NULL)
    errx(EXIT_FAILURE, "Could not open %s : %s\n",
	 trace_filename, strerror(errno));

  backtrace_FILE_ptr = fopen(backtrace_filename, "w");
  if (backtrace_FILE_ptr == NULL)
    errx(EXIT_FAILURE, "Could not open %s : %s\n",
	 backtrace_filename, strerror(errno));
  
  backtrace_fd = fileno(backtrace_FILE_ptr);
  if (backtrace_fd == -1)
    errx(EXIT_FAILURE, "Could not open %s : %s\n",
	 backtrace_filename, strerror(errno));

}

 __attribute__((destructor(0)))
void vfc_tracer_end(void) {
   fclose(trace_FILE_ptr);
   fclose(backtrace_FILE_ptr);
}

/* sets verificarlo precision and mode. Returns 0 on success. */
int vfc_set_precision_and_mode(unsigned int precision, int mode) {
	if (mode < 0 || mode > 3)
		return -1;

    verificarlo_precision = precision;
    verificarlo_mcamode = mode;

    switch (verificarlo_backend) {
    case MCABACKEND_MPFR:
      vfc_select_interface_mpfr();
      break;
    case MCABACKEND_QUAD:
      vfc_select_interface_quad();
      break;
    case BACKEND_BITMASK:
      vfc_select_interface_bitmask();
      break;
    default:
      perror("Invalid backend name in backend setting\n");
      exit(-1);      
    }

    return 0;
}

/* vfc_init is run when loading vfcwrapper and initializes vfc libraries */
__attribute__((constructor(0)))
static void vfc_init (void)
{
    char * endptr;

    /* If VERIFICARLO_PRECISION is set, try to parse it */
    char * precision = getenv(VERIFICARLO_PRECISION);
    if (precision != NULL) {
        errno = 0;
        int val = strtol(precision, &endptr, 10);
        if (errno != 0 || val <= 0) {
            /* Invalid value provided */
            fprintf(stderr, VERIFICARLO_PRECISION
                   " invalid value provided, defaulting to default\n");
        } else {
            verificarlo_precision = val;
        }
    }

     /* If VERIFICARLO_MCAMODE is set, try to parse it */
    char * mode = getenv(VERIFICARLO_MCAMODE);
    if (mode != NULL) {
        if (strcmp("IEEE", mode) == 0) {
            verificarlo_mcamode = MCAMODE_IEEE;
        }
        else if (strcmp("MCA", mode) == 0) {
            verificarlo_mcamode = MCAMODE_MCA;
        }
        else if (strcmp("PB", mode) == 0) {
            verificarlo_mcamode = MCAMODE_PB;
        }
        else if (strcmp("RR", mode) == 0) {
            verificarlo_mcamode = MCAMODE_RR;
        } else {
            /* Invalid value provided */
            fprintf(stderr, VERIFICARLO_MCAMODE
                   " invalid value provided, defaulting to default\n");
        }
    }

    /* If VERIFICARLO_BACKEND is set, try to parse it */
    char * backend = getenv(VERIFICARLO_BACKEND);
    if (backend != NULL) {
      if (strcmp("QUAD", backend) == 0) {
        verificarlo_backend = MCABACKEND_QUAD;
      }
      else if (strcmp("MPFR", backend) == 0) {
        verificarlo_backend = MCABACKEND_MPFR;
      }
      else if (strcmp("BITMASK", backend) == 0) {
	verificarlo_backend = BACKEND_BITMASK;     
      } else {
        /* Invalid value provided */
        fprintf(stderr, VERIFICARLO_BACKEND
                " invalid value provided, defaulting to default %s\n",backend);
      }
    }

    char *bitmask_mode = getenv(VERIFICARLO_BITMASK_MODE);
    if (bitmask_mode != NULL) {
      if (strcmp("ZERO", bitmask_mode) == 0) {
        verificarlo_bitmask_mode = BITMASK_MODE_ZERO;
      }
      else if (strcmp("INV", bitmask_mode) == 0) {
        verificarlo_bitmask_mode = BITMASK_MODE_INV;
      }
      else if (strcmp("RAND", bitmask_mode) == 0) {
        verificarlo_bitmask_mode = BITMASK_MODE_RAND;
      } else {
        /* Invalid value provided */
        fprintf(stderr, VERIFICARLO_BITMASK_MODE
                " invalid value provided, defaulting to default %s\n",bitmask_mode);
      }
      
    }
    
    /* seed the backends */
    vfc_seed();

    /* set precision and mode */
    vfc_set_precision_and_mode(verificarlo_precision, verificarlo_mcamode);

    /* init files for tracing */
    vfc_tracer();

}

typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));

/* Arithmetic vector wrappers */

double2 _2xdoubleadd(double2 a, double2 b) {
    double2 c;

    c[0] = _vfc_current_mca_interface.doubleadd(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doubleadd(a[1],b[1]);
    return c;
}

double2 _2xdoublesub(double2 a, double2 b) {
    double2 c;

    c[0] = _vfc_current_mca_interface.doublesub(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublesub(a[1],b[1]);
    return c;
}

double2 _2xdoublemul(double2 a, double2 b) {
    double2 c;

    c[0] = _vfc_current_mca_interface.doublemul(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublemul(a[1],b[1]);
    return c;
}

double2 _2xdoublediv(double2 a, double2 b) {
    double2 c;

    c[0] = _vfc_current_mca_interface.doublediv(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublediv(a[1],b[1]);
    return c;
}


/*********************************************************/

double4 _4xdoubleadd(double4 a, double4 b) {
    double4 c;

    c[0] = _vfc_current_mca_interface.doubleadd(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doubleadd(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.doubleadd(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.doubleadd(a[3],b[3]);
    return c;
}

double4 _4xdoublesub(double4 a, double4 b) {
    double4 c;

    c[0] = _vfc_current_mca_interface.doublesub(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublesub(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.doublesub(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.doublesub(a[3],b[3]);
    return c;
}

double4 _4xdoublemul(double4 a, double4 b) {
    double4 c;

    c[0] = _vfc_current_mca_interface.doublemul(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublemul(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.doublemul(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.doublemul(a[3],b[3]);
    return c;
}

double4 _4xdoublediv(double4 a, double4 b) {
    double4 c;

    c[0] = _vfc_current_mca_interface.doublediv(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.doublediv(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.doublediv(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.doublediv(a[3],b[3]);
    return c;
}


/*********************************************************/


float2 _2xfloatadd(float2 a, float2 b) {
    float2 c;

    c[0] = _vfc_current_mca_interface.floatadd(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatadd(a[1],b[1]);
    return c;
}

float2 _2xfloatsub(float2 a, float2 b) {
    float2 c;

    c[0] = _vfc_current_mca_interface.floatsub(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatsub(a[1],b[1]);
    return c;
}

float2 _2xfloatmul(float2 a, float2 b) {
    float2 c;

    c[0] = _vfc_current_mca_interface.floatmul(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatmul(a[1],b[1]);
    return c;
}

float2 _2xfloatdiv(float2 a, float2 b) {
    float2 c;

    c[0] = _vfc_current_mca_interface.floatdiv(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatdiv(a[1],b[1]);
    return c;
}

float4 _4xfloatadd(float4 a, float4 b) {
    float4 c;

    c[0] = _vfc_current_mca_interface.floatadd(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatadd(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.floatadd(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.floatadd(a[3],b[3]);
    return c;
}

float4 _4xfloatsub(float4 a, float4 b) {
    float4 c;

    c[0] = _vfc_current_mca_interface.floatsub(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatsub(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.floatsub(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.floatsub(a[3],b[3]);
    return c;
}

float4 _4xfloatmul(float4 a, float4 b) {
    float4 c;

    c[0] = _vfc_current_mca_interface.floatmul(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatmul(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.floatmul(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.floatmul(a[3],b[3]);
    return c;
}

float4 _4xfloatdiv(float4 a, float4 b) {
    float4 c;

    c[0] = _vfc_current_mca_interface.floatdiv(a[0],b[0]);
    c[1] = _vfc_current_mca_interface.floatdiv(a[1],b[1]);
    c[2] = _vfc_current_mca_interface.floatdiv(a[2],b[2]);
    c[3] = _vfc_current_mca_interface.floatdiv(a[3],b[3]);
    return c;
}

static uint64_t get_timestamp(void) {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  uint64_t usecs = tv.tv_sec*1000000ull+tv.tv_usec;
  return usecs;
}

/* Probes used by the veritracer pass */

/* Probes used for the text format */ 

/* int */

void _veritracer_probe_int32(int32_t value, int32_t* value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "int32 %lu %lu %p %d\n", get_timestamp(), hash_LI, value_ptr, value);
}

void _veritracer_probe_int64(int64_t value, int64_t* value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "int64 %lu %lu %p %ld\n", get_timestamp(), hash_LI, value_ptr, value);
}

/* binary32 */

void _veritracer_probe_binary32(float value, float* value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", get_timestamp(), hash_LI, value_ptr, value);
}

void _veritracer_probe_binary32_ptr(float *value, float* value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", get_timestamp(), hash_LI, value_ptr, *value);
}

void _veritracer_probe_2xbinary32(float2 value, float* value_ptr, uint64_t hash_LI[2]) {
  const int N = 2;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", timestamp, hash_LI[i], value_ptr, value[i]);
}

void _veritracer_probe_4xbinary32(float4 value, float4* value_ptr, uint64_t hash_LI[4]) {
  const int N = 4;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary32 %lu %lu %p %0.6a\n", timestamp, hash_LI[i], value_ptr, value[i]);
}

/* binary64 */

void _veritracer_probe_binary64(double value, double* value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", get_timestamp(), hash_LI, value_ptr, value);
}

void _veritracer_probe_binary64_ptr(double *value, double *value_ptr, uint64_t hash_LI) {
  fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", get_timestamp(), hash_LI, value_ptr, *value);
}
				   
void _veritracer_probe_2xbinary64(double2 value, double* value_ptr, uint64_t hash_LI[2]) {
  const int N = 2;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", timestamp, hash_LI[i], value_ptr, value[i]);
}

void _veritracer_probe_4xbinary64(double4 value, double* value_ptr, uint64_t hash_LI[4]) {
  const int N = 4;
  uint64_t timestamp = get_timestamp();
  for (int i = 0; i < N; i++)
    fprintf(trace_FILE_ptr, "binary64 %lu %lu %p %0.13a\n", timestamp, hash_LI[i], value_ptr, value[i]);
}

/* Probes used for the binary format */ 

/* int */

void _veritracer_probe_int32_binary(int32_t value, int32_t* value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_int32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_int32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_int64_binary(int64_t value, int64_t *value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_int64_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;  
  fwrite(&fmt, sizeof_int64_fmt, 1, trace_FILE_ptr);
}

/* binary32 */

void _veritracer_probe_binary32_binary(float value, float* value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_binary32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;  
  fwrite(&fmt, sizeof_binary32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_binary32_binary_ptr(float *value, float* value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_binary32_fmt_t fmt;
  fmt.sizeof_value = sizeof(value);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = *value;  
  fwrite(&fmt, sizeof_binary32_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_2xbinary32_binary(float2 value, float* value_ptr, uint64_t hash_LI[2]) {
  const int N = 2;
  struct veritracer_probe_binary32_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for(int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(float);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary32_fmt, N, trace_FILE_ptr);
}

void _veritracer_probe_4xbinary32_binary(float4 value, float* value_ptr, uint64_t hash_LI[4]) {
  const int N = 4;
  struct veritracer_probe_binary32_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();
  for(int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(float);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary32_fmt, N, trace_FILE_ptr);
}

/* binary64 */

void _veritracer_probe_binary64_binary(double value, double* value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_binary64_fmt_t fmt;
  fmt.sizeof_value = sizeof(double);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = value;
  fwrite(&fmt, sizeof_binary64_fmt, 1, trace_FILE_ptr);
}


void _veritracer_probe_binary64_binary_ptr(double *value, double* value_ptr, uint64_t hash_LI) {
  struct veritracer_probe_binary64_fmt_t fmt;
  fmt.sizeof_value = sizeof(double);
  fmt.timestamp = get_timestamp();
  fmt.value_ptr = value_ptr;
  fmt.hash_LI = hash_LI;
  fmt.value = *value;
  fwrite(&fmt, sizeof_binary64_fmt, 1, trace_FILE_ptr);
}

void _veritracer_probe_2xbinary64_binary(double2 value, double* value_ptr, uint64_t hash_LI[2]) {
  const int N = 2;
  struct veritracer_probe_binary64_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();  
  for(int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(double);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary64_fmt, N, trace_FILE_ptr);
}

void _veritracer_probe_4xbinary64_binary(double4 value, double* value_ptr, uint64_t hash_LI[4]) {
  const int N = 4;
  struct veritracer_probe_binary64_fmt_t fmt[N];
  uint64_t timestamp = get_timestamp();  
  for(int i = 0; i < N; i++) {
    fmt[i].sizeof_value = sizeof(double);
    fmt[i].timestamp = timestamp;
    fmt[i].value_ptr = value_ptr;
    fmt[i].hash_LI = hash_LI[i];
    fmt[i].value = value[i];
  }
  fwrite(&fmt, sizeof_binary64_fmt, N, trace_FILE_ptr);
}

/* backtrace */

void get_backtrace(uint64_t hash_LI) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count = 20 + sizeof(backtrace_separator)-1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer, "%020lu%s", hash_LI, backtrace_separator);
  write(backtrace_fd, string_buffer, count);
}

void get_backtrace_x2(uint64_t hash_LI[2]) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count = 20 + 1 + 20 + sizeof(backtrace_separator) - 1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer, "%020lu.%020lu%s", hash_LI[0], hash_LI[1], backtrace_separator);
  write(backtrace_fd, string_buffer, count);
}

void get_backtrace_x4(uint64_t hash_LI[4]) {
  int nptrs = backtrace(backtrace_buffer, SIZE_MAX_BACKTRACE);
  ssize_t count = 20 + 1 + 20 + 1 + 20 + 1 + 20 + sizeof(backtrace_separator) - 1;
  backtrace_symbols_fd(backtrace_buffer, nptrs, backtrace_fd);
  sprintf(string_buffer,"%020lu.%020lu.%020lu.%020lu%s",
	  hash_LI[0], hash_LI[1], hash_LI[2], hash_LI[3], backtrace_separator);
  write(backtrace_fd, string_buffer, sizeof(string_buffer)-1);
}
