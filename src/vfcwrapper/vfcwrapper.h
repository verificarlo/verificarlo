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

#include <stdint.h>

/* define the available MCA modes of operation */
#define MCAMODE_IEEE 0
#define MCAMODE_MCA  1
#define MCAMODE_PB   2
#define MCAMODE_RR   3

/* define the available MCA backends */
#define MCABACKEND_QUAD 0
#define MCABACKEND_MPFR 1
#define MCABACKEND_RDROUND 2
#define BACKEND_BITMASK 3

/* define the available bitmask mode */
#define BITMASK_MODE_ZERO 0
#define BITMASK_MODE_INV  1
#define BITMASK_MODE_RAND 2

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

struct __attribute__((packed)) veritracer_probe_binary32_fmt_t {
  uint32_t sizeofValue;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  float value;
};


struct __attribute__((packed)) veritracer_probe_binary64_fmt_t {
  uint32_t sizeofValue;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  double value;
};

struct __attribute__((packed)) veritracer_probe_int32_fmt_t {
  uint32_t sizeofValue;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  int32_t value;
};

struct __attribute__((packed)) veritracer_probe_int64_fmt_t {
  uint32_t sizeofValue;
  uint64_t timestamp;
  void *value_ptr;
  uint64_t hash_LI;
  int64_t value;
};

int sizeof_binary32_fmt = sizeof(struct veritracer_probe_binary32_fmt_t);
int sizeof_binary64_fmt = sizeof(struct veritracer_probe_binary64_fmt_t);
int sizeof_int32_fmt = sizeof(struct veritracer_probe_int32_fmt_t);
int sizeof_int64_fmt = sizeof(struct veritracer_probe_int64_fmt_t);
