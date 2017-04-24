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

/* define the available MCA modes of operation */
#define MCAMODE_IEEE 0
#define MCAMODE_MCA  1
#define MCAMODE_PB   2
#define MCAMODE_RR   3

/* define the available MCA backends */
#define MCABACKEND_QUAD 0
#define MCABACKEND_MPFR 1
#define MCABACKEND_RDROUND 2
/* seeds all the MCA backends */
void vfc_seed(void);

/* sets verificarlo precision and mode. Returns 0 on success. */
int vfc_set_precision_and_mode(unsigned int precision, int mode);

/* MCA backend interface */
struct mca_interface_t {
    float (*floatadd)(float, float);
    float (*floatsub)(float, float);
    float (*floatmul)(float, float);
    float (*floatdiv)(float, float);

    double (*doubleadd)(double, double);
    double (*doublesub)(double, double);
    double (*doublemul)(double, double);
    double (*doublediv)(double, double);

    void (*seed)(void);
    int (*set_mca_mode)(int);
    int (*set_mca_precision)(int);
};
