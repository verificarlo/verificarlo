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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "vfcwrapper.h"

#include "libmca-mpfr.h"
#include "libmca-quad.h"

#define VERIFICARLO_PRECISION "VERIFICARLO_PRECISION"
#define VERIFICARLO_MCAMODE "VERIFICARLO_MCAMODE"
#define VERIFICARLO_BACKEND "VERIFICARLO_BACKEND"
#define VERIFICARLO_PRECISION_DEFAULT 53
#define VERIFICARLO_MCAMODE_DEFAULT MCAMODE_MCA
#define VERIFICARLO_BACKEND_DEFAULT MCABACKEND_MPFR


/* Set default values for MCA*/
int verificarlo_precision = VERIFICARLO_PRECISION_DEFAULT;
int verificarlo_mcamode = VERIFICARLO_MCAMODE_DEFAULT;
int verificarlo_backend = VERIFICARLO_BACKEND_DEFAULT;

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




/* seeds all the MCA backends */
void vfc_seed(void) {
    mpfr_mca_interface.seed();
    quad_mca_interface.seed();
}

/* sets verificarlo precision and mode. Returns 0 on success. */
int vfc_set_precision_and_mode(unsigned int precision, int mode) {
	if (mode < 0 || mode > 3)
		return -1;

    verificarlo_precision = precision;
    verificarlo_mcamode = mode;

    /* Choose the required backend */
    if (verificarlo_backend == MCABACKEND_MPFR) {
      vfc_select_interface_mpfr();
    }
    else if (verificarlo_backend == MCABACKEND_QUAD){
      vfc_select_interface_quad();
    }
    else {
    	perror("Invalid backend name in backend setting\n");
	exit(-1);
    }
    return 0;
}

/* vfc_init is run when loading vfcwrapper and initializes vfc libraries */
__attribute__((constructor))
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
      }else {
        /* Invalid value provided */
        fprintf(stderr, VERIFICARLO_BACKEND
                " invalid value provided, defaulting to default\n");
      }
    }

    /* seed the backends */
    vfc_seed();

    /* set precision and mode */
    vfc_set_precision_and_mode(verificarlo_precision, verificarlo_mcamode);
}

typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef bool bool2 __attribute__((ext_vector_type(2)));
typedef bool bool4 __attribute__((ext_vector_type(4)));

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

