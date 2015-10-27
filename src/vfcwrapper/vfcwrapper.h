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

#define MCAMODE_IEEE 0
#define MCAMODE_MCA  1
#define MCAMODE_PB   2
#define MCAMODE_RR   3

struct mca_interface_t {
    int (*floateq)(float, float);
    int (*floatne)(float, float);
    int (*floatlt)(float, float);
    int (*floatgt)(float, float);
    int (*floatle)(float, float);
    int (*floatge)(float, float);
    float (*floatadd)(float, float);
    float (*floatsub)(float, float);
    float (*floatmul)(float, float);
    float (*floatdiv)(float, float);

    int (*doubleeq)(double, double);
    int (*doublene)(double, double);
    int (*doublelt)(double, double);
    int (*doublegt)(double, double);
    int (*doublele)(double, double);
    int (*doublege)(double, double);
    double (*doubleadd)(double, double);
    double (*doublesub)(double, double);
    double (*doublemul)(double, double);
    double (*doublediv)(double, double);

    void (*seed)(void);
    int (*set_mca_mode)(int);
    int (*set_mca_precision)(int);
};
