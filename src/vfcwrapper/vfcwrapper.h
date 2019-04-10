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

    bool (*float_false)(float, float);
    bool (*float_oeq)(float, float);
    bool (*float_ogt)(float, float);
    bool (*float_oge)(float, float);
    bool (*float_olt)(float, float);
    bool (*float_ole)(float, float);
    bool (*float_one)(float, float);
    bool (*float_ord)(float, float);
    bool (*float_ueq)(float, float);
    bool (*float_ugt)(float, float);
    bool (*float_uge)(float, float);
    bool (*float_ult)(float, float);
    bool (*float_ule)(float, float);
    bool (*float_une)(float, float);
    bool (*float_uno)(float, float);
    bool (*float_true)(float, float);

    bool (*double_false)(double, double);
    bool (*double_oeq)(double, double);
    bool (*double_ogt)(double, double);
    bool (*double_oge)(double, double);
    bool (*double_olt)(double, double);
    bool (*double_ole)(double, double);
    bool (*double_one)(double, double);
    bool (*double_ord)(double, double);
    bool (*double_ueq)(double, double);
    bool (*double_ugt)(double, double);
    bool (*double_uge)(double, double);
    bool (*double_ult)(double, double);
    bool (*double_ule)(double, double);
    bool (*double_une)(double, double);
    bool (*double_uno)(double, double);
    bool (*double_true)(double, double);

    void (*seed)(void);
    int (*set_mca_mode)(int);
    int (*set_mca_precision)(int);
};
