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

#include<mcalib.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

#include "vfcwrapper.h"

#define VERIFICARLO_PRECISION "VERIFICARLO_PRECISION"
#define VERIFICARLO_MCAMODE "VERIFICARLO_MCAMODE"
#define VERIFICARLO_PRECISION_DEFAULT 53
#define VERIFICARLO_MCAMODE_DEFAULT MCAMODE_MCA;

/* Set default values for MCA*/
int verificarlo_precision = VERIFICARLO_PRECISION_DEFAULT;
int verificarlo_mcamode = VERIFICARLO_MCAMODE_DEFAULT;

struct mca_interface_t * current_mca_interface;

void seed_all() {
    mpfr_mca_interface.seed();
}

void select_interface_mpfr(void) {
    current_mca_interface = &mpfr_mca_interface;
    current_mca_interface->set_mca_precision(verificarlo_precision);
    current_mca_interface->set_mca_mode(verificarlo_mcamode);
}

__attribute__((constructor)) void begin (void)
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

    seed_all();
    select_interface_mpfr();
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

    c[0] = current_mca_interface->doubleadd(a[0],b[0]);
    c[1] = current_mca_interface->doubleadd(a[1],b[1]);
    return c;
}

double2 _2xdoublesub(double2 a, double2 b) {
    double2 c;

    c[0] = current_mca_interface->doublesub(a[0],b[0]);
    c[1] = current_mca_interface->doublesub(a[1],b[1]);
    return c;
}

double2 _2xdoublemul(double2 a, double2 b) {
    double2 c;

    c[0] = current_mca_interface->doublemul(a[0],b[0]);
    c[1] = current_mca_interface->doublemul(a[1],b[1]);
    return c;
}

double2 _2xdoublediv(double2 a, double2 b) {
    double2 c;

    c[0] = current_mca_interface->doublediv(a[0],b[0]);
    c[1] = current_mca_interface->doublediv(a[1],b[1]);
    return c;
}

double4 _4xdoubleadd(double4 a, double4 b) {
    double4 c;

    c[0] = current_mca_interface->doubleadd(a[0],b[0]);
    c[1] = current_mca_interface->doubleadd(a[1],b[1]);
    c[2] = current_mca_interface->doubleadd(a[2],b[2]);
    c[3] = current_mca_interface->doubleadd(a[3],b[3]);
    return c;
}

double4 _4xdoublesub(double4 a, double4 b) {
    double4 c;

    c[0] = current_mca_interface->doublesub(a[0],b[0]);
    c[1] = current_mca_interface->doublesub(a[1],b[1]);
    c[2] = current_mca_interface->doublesub(a[2],b[2]);
    c[3] = current_mca_interface->doublesub(a[3],b[3]);
    return c;
}

double4 _4xdoublemul(double4 a, double4 b) {
    double4 c;

    c[0] = current_mca_interface->doublemul(a[0],b[0]);
    c[1] = current_mca_interface->doublemul(a[1],b[1]);
    c[2] = current_mca_interface->doublemul(a[2],b[2]);
    c[3] = current_mca_interface->doublemul(a[3],b[3]);
    return c;
}

double4 _4xdoublediv(double4 a, double4 b) {
    double4 c;

    c[0] = current_mca_interface->doublediv(a[0],b[0]);
    c[1] = current_mca_interface->doublediv(a[1],b[1]);
    c[2] = current_mca_interface->doublediv(a[2],b[2]);
    c[3] = current_mca_interface->doublediv(a[3],b[3]);
    return c;
}

float2 _2xfloatadd(float2 a, float2 b) {
    float2 c;

    c[0] = current_mca_interface->floatadd(a[0],b[0]);
    c[1] = current_mca_interface->floatadd(a[1],b[1]);
    return c;
}

float2 _2xfloatsub(float2 a, float2 b) {
    float2 c;

    c[0] = current_mca_interface->floatsub(a[0],b[0]);
    c[1] = current_mca_interface->floatsub(a[1],b[1]);
    return c;
}

float2 _2xfloatmul(float2 a, float2 b) {
    float2 c;

    c[0] = current_mca_interface->floatmul(a[0],b[0]);
    c[1] = current_mca_interface->floatmul(a[1],b[1]);
    return c;
}

float2 _2xfloatdiv(float2 a, float2 b) {
    float2 c;

    c[0] = current_mca_interface->floatdiv(a[0],b[0]);
    c[1] = current_mca_interface->floatdiv(a[1],b[1]);
    return c;
}

float4 _4xfloatadd(float4 a, float4 b) {
    float4 c;

    c[0] = current_mca_interface->floatadd(a[0],b[0]);
    c[1] = current_mca_interface->floatadd(a[1],b[1]);
    c[2] = current_mca_interface->floatadd(a[2],b[2]);
    c[3] = current_mca_interface->floatadd(a[3],b[3]);
    return c;
}

float4 _4xfloatsub(float4 a, float4 b) {
    float4 c;

    c[0] = current_mca_interface->floatsub(a[0],b[0]);
    c[1] = current_mca_interface->floatsub(a[1],b[1]);
    c[2] = current_mca_interface->floatsub(a[2],b[2]);
    c[3] = current_mca_interface->floatsub(a[3],b[3]);
    return c;
}

float4 _4xfloatmul(float4 a, float4 b) {
    float4 c;

    c[0] = current_mca_interface->floatmul(a[0],b[0]);
    c[1] = current_mca_interface->floatmul(a[1],b[1]);
    c[2] = current_mca_interface->floatmul(a[2],b[2]);
    c[3] = current_mca_interface->floatmul(a[3],b[3]);
    return c;
}

float4 _4xfloatdiv(float4 a, float4 b) {
    float4 c;

    c[0] = current_mca_interface->floatdiv(a[0],b[0]);
    c[1] = current_mca_interface->floatdiv(a[1],b[1]);
    c[2] = current_mca_interface->floatdiv(a[2],b[2]);
    c[3] = current_mca_interface->floatdiv(a[3],b[3]);
    return c;
}

/* Comparison vector wrappers */

bool2 _2xdoubleeq(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doubleeq(a[0],b[0]);
    c[1] = current_mca_interface->doubleeq(a[1],b[1]);
    return c;
}

bool2 _2xdoublene(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doublene(a[0],b[0]);
    c[1] = current_mca_interface->doublene(a[1],b[1]);
    return c;
}

bool2 _2xdoublelt(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doublelt(a[0],b[0]);
    c[1] = current_mca_interface->doublelt(a[1],b[1]);
    return c;
}

bool2 _2xdoublegt(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doublegt(a[0],b[0]);
    c[1] = current_mca_interface->doublegt(a[1],b[1]);
    return c;
}

bool2 _2xdoublele(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doublele(a[0],b[0]);
    c[1] = current_mca_interface->doublele(a[1],b[1]);
    return c;
}


bool2 _2xdoublege(double2 a, double2 b) {
    bool2 c;
    c[0] = current_mca_interface->doublege(a[0],b[0]);
    c[1] = current_mca_interface->doublege(a[1],b[1]);
    return c;
}

bool4 _4xdoubleeq(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doubleeq(a[0],b[0]);
    c[1] = current_mca_interface->doubleeq(a[1],b[1]);
    c[2] = current_mca_interface->doubleeq(a[2],b[2]);
    c[3] = current_mca_interface->doubleeq(a[3],b[3]);
    return c;
}

bool4 _4xdoublene(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doublene(a[0],b[0]);
    c[1] = current_mca_interface->doublene(a[1],b[1]);
    c[2] = current_mca_interface->doublene(a[2],b[2]);
    c[3] = current_mca_interface->doublene(a[3],b[3]);
    return c;
}

bool4 _4xdoublelt(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doublelt(a[0],b[0]);
    c[1] = current_mca_interface->doublelt(a[1],b[1]);
    c[2] = current_mca_interface->doublelt(a[2],b[2]);
    c[3] = current_mca_interface->doublelt(a[3],b[3]);
    return c;
}

bool4 _4xdoublegt(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doublegt(a[0],b[0]);
    c[1] = current_mca_interface->doublegt(a[1],b[1]);
    c[2] = current_mca_interface->doublegt(a[2],b[2]);
    c[3] = current_mca_interface->doublegt(a[3],b[3]);
    return c;
}

bool4 _4xdoublele(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doublele(a[0],b[0]);
    c[1] = current_mca_interface->doublele(a[1],b[1]);
    c[2] = current_mca_interface->doublele(a[2],b[2]);
    c[3] = current_mca_interface->doublele(a[3],b[3]);
    return c;
}

bool4 _4xdoublege(double4 a, double4 b) {
    bool4 c;
    c[0] = current_mca_interface->doublege(a[0],b[0]);
    c[1] = current_mca_interface->doublege(a[1],b[1]);
    c[2] = current_mca_interface->doublege(a[2],b[2]);
    c[3] = current_mca_interface->doublege(a[3],b[3]);
    return c;
}

bool2 _2xfloateq(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floateq(a[0],b[0]);
    c[1] = current_mca_interface->floateq(a[1],b[1]);
    return c;
}

bool2 _2xfloatne(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floatne(a[0],b[0]);
    c[1] = current_mca_interface->floatne(a[1],b[1]);
    return c;
}

bool2 _2xfloatlt(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floatlt(a[0],b[0]);
    c[1] = current_mca_interface->floatlt(a[1],b[1]);
    return c;
}

bool2 _2xfloatgt(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floatgt(a[0],b[0]);
    c[1] = current_mca_interface->floatgt(a[1],b[1]);
    return c;
}

bool2 _2xfloatle(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floatle(a[0],b[0]);
    c[1] = current_mca_interface->floatle(a[1],b[1]);
    return c;
}

bool2 _2xfloatge(float2 a, float2 b) {
    bool2 c;
    c[0] = current_mca_interface->floatge(a[0],b[0]);
    c[1] = current_mca_interface->floatge(a[1],b[1]);
    return c;
}

bool4 _4xfloateq(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floateq(a[0],b[0]);
    c[1] = current_mca_interface->floateq(a[1],b[1]);
    c[2] = current_mca_interface->floateq(a[2],b[2]);
    c[3] = current_mca_interface->floateq(a[3],b[3]);
    return c;
}

bool4 _4xfloatne(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floatne(a[0],b[0]);
    c[1] = current_mca_interface->floatne(a[1],b[1]);
    c[2] = current_mca_interface->floatne(a[2],b[2]);
    c[3] = current_mca_interface->floatne(a[3],b[3]);
    return c;
}

bool4 _4xfloatlt(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floatlt(a[0],b[0]);
    c[1] = current_mca_interface->floatlt(a[1],b[1]);
    c[2] = current_mca_interface->floatlt(a[2],b[2]);
    c[3] = current_mca_interface->floatlt(a[3],b[3]);
    return c;
}

bool4 _4xfloatgt(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floatgt(a[0],b[0]);
    c[1] = current_mca_interface->floatgt(a[1],b[1]);
    c[2] = current_mca_interface->floatgt(a[2],b[2]);
    c[3] = current_mca_interface->floatgt(a[3],b[3]);
    return c;
}

bool4 _4xfloatle(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floatle(a[0],b[0]);
    c[1] = current_mca_interface->floatle(a[1],b[1]);
    c[2] = current_mca_interface->floatle(a[2],b[2]);
    c[3] = current_mca_interface->floatle(a[3],b[3]);
    return c;
}

bool4 _4xfloatge(float4 a, float4 b) {
    bool4 c;
    c[0] = current_mca_interface->floatge(a[0],b[0]);
    c[1] = current_mca_interface->floatge(a[1],b[1]);
    c[2] = current_mca_interface->floatge(a[2],b[2]);
    c[3] = current_mca_interface->floatge(a[3],b[3]);
    return c;
}
