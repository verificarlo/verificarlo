/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
/*
 * This file defines "vfc_probes", a hashtable-based structure which can be used
 * to place "probes" in a code and store the different values of test variables.
 * These test results can then be exported in a CSV file, and used to generate a
 * Verificarlo test report.
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interflop/hashmap/vfc_hashmap.h"

#ifndef VAR_NAME
#define VAR_NAME(var) #var // Simply returns the name of var into a string
#endif

#ifdef __cplusplus
extern "C" {
#endif

// A probe containing a double value as well as its key, which is needed when
// dumping the probes. Optionally, an accuracy threshold can be defined : it
// will be re-used in the preprocessing to (un)validate the probe.
struct vfc_probe_node {
  char *key;
  double value;

  double accuracyThreshold;
  char *mode;
};

typedef struct vfc_probe_node vfc_probe_node;

// The probes structure. It simply acts as a wrapper for a Verificarlo hashmap.
struct vfc_probes {
  vfc_hashmap_t map;
};

typedef struct vfc_probes vfc_probes;

// Initialize an empty vfc_probes instance
vfc_probes vfc_init_probes();

// Free all probes
void vfc_free_probes(vfc_probes *probes);

// Helper function to generate the key from test and variable name
char *gen_probe_key(char *testName, char *varName);

// Helper function to detect forbidden character ',' in the keys
void validate_probe_key(char *str);

// Probe kernel function that supports checks and use any mode (relative /
// absolute). This probably won't be called directly by the user.
int vfc_probe_kernel(vfc_probes *probes, char *testName, char *varName,
                     double val, double accuracyThreshold, char *mode);

// Add a new probe. If an issue with the key is detected (forbidden characters
// or a duplicate key), an error will be thrown. (no check)
int vfc_probe(vfc_probes *probes, char *testName, char *varName, double val);

// Similar to vfc_probe, but with an optional accuracy threshold (absolute
// check).
int vfc_probe_check(vfc_probes *probes, char *testName, char *varName,
                    double val, double accuracyThreshold);

// Similar to vfc_probe, but with an optional accuracy threshold (absolute
// check).
int vfc_probe_check_relative(vfc_probes *probes, char *testName, char *varName,
                             double val, double accuracyThreshold);

// Return the number of probes stored in the hashmap
unsigned int vfc_num_probes(vfc_probes *probes);

// Dump probes in a .csv file (the double values are converted to hex), then
// free it.
int vfc_dump_probes(vfc_probes *probes);

// Fortran wrappers

int vfc_probe_f(vfc_probes *probes, char *testName, char *varName, double *val);

int vfc_probe_check_f(vfc_probes *probes, char *testName, char *varName,
                      double *val, double *accuracyThreshold);

int vfc_probe_check_relative_f(vfc_probes *probes, char *testName,
                               char *varName, double *val,
                               double *accuracyThreshold);

#ifdef __cplusplus
}
#endif
