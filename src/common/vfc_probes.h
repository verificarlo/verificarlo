/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015-2021                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

/*
 * This file defines "vfc_probes", a hashtable-based structure which can be used
 * to place "probes" in a code and store the different values of test variables.
 * These test results can then be exported in a CSV file, and used to generate a
 * Verificarlo test report.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vfc_hashmap.h>

#ifndef VAR_NAME
#define VAR_NAME(var) #var // Simply returns the name of var into a string
#endif

// A probe containing a double value as well as its key, which is needed when
// dumping the probes
struct vfc_probe_node {
  char *key;
  double value;
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

// Add a new probe. If an issue with the key is detected (forbidden characters
// or a duplicate key), an error will be thrown.
int vfc_probe(vfc_probes *probes, char *testName, char *varName, double val);

// Return the number of probes stored in the hashmap
unsigned int vfc_num_probes(vfc_probes *probes);

// Dump probes in a .csv file (the double values are converted to hex), then
// free it.
int vfc_dump_probes(vfc_probes *probes);

// Fortran wrapper
int vfc_probe_f(vfc_probes *probes, char *testName, char *varName, double *val);
