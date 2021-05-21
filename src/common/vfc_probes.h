/*
 * This file defines "vfc_probes", a hashtable-based structure which can be used
 * to place "probes" in a code and store the different values of test variables.
 * These test results can then be exported in a CSV file, and used to generate a
 * Verificarlo test report.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vfc_hashmap.h>

#ifndef __VFC_PROBES_H__
#define __VFC_PROBES_H__

#define __VFC_PROBES_HEADER__

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

// Iniialize an empty vfc_probes instance
vfc_probes vfc_init_probes() {
  vfc_probes probes;
  probes.map = vfc_hashmap_create();

  return probes;
}

// Free all probes
void vfc_free_probes(vfc_probes *probes) {

  // Before freeing the map, iterate manually over all items to free the keys
  vfc_probe_node *probe = NULL;
  for (int i = 0; i < probes->map->capacity; i++) {
    probe = (vfc_probe_node *)get_value_at(probes->map->items, i);
    if (probe != NULL) {
      if (probe->key != NULL) {
        free(probe->key);
      }
    }
  }

  vfc_hashmap_free(probes->map);
}

// Helper function to generate the key from test and variable name
char *gen_probe_key(char *testName, char *varName) {
  char *key = (char *)malloc(strlen(testName) + strlen(varName) + 2);
  strcpy(key, testName);
  strcat(key, ",");
  strcat(key, varName);

  return key;
}

// Helper function to detect forbidden character ',' in the keys
void validate_probe_key(char *str) {
  unsigned int len = strlen(str);

  for (unsigned int i = 0; i < len; i++) {
    if (str[i] == ',') {
      fprintf(stderr,
              "Error [verificarlo]: One of your probes has a ',' in its test \
                or variable name (\"%s\"), which is forbidden\n",
              str);
      exit(1);
    }
  }
}

// Add a new probe. If an issue with the key is detected (forbidden characters
// or a duplicate key), an error will be thrown.
int vfc_probe(vfc_probes *probes, char *testName, char *varName,
                  double val) {

  if (probes == NULL) {
    return 1;
  }

  // Make sure testName and varName don't contain any ',', which would
  // interfere with the key/CSV encoding
  validate_probe_key(testName);
  validate_probe_key(varName);

  // Get the key, which is : testName + "," + varName
  char *key = gen_probe_key(testName, varName);

  // Look for a duplicate key
  vfc_probe_node *oldProbe = (vfc_probe_node *)vfc_hashmap_get(
      probes->map, vfc_hashmap_str_function(key));

  if (oldProbe != NULL) {
    if (strcmp(key, oldProbe->key) == 0) {
      fprintf(stderr,
              "Error [verificarlo]: you have a duplicate error with one of \
                your probes (\"%s\"). Please make sure to use different names.\n",
              key);
      exit(1);
    }
  }

  // Insert the element in the hashmap
  vfc_probe_node *newProbe = (vfc_probe_node *)malloc(sizeof(vfc_probe_node));
  newProbe->key = key;
  newProbe->value = val;

  vfc_hashmap_insert(probes->map, vfc_hashmap_str_function(key), newProbe);

  return 0;
}

// Remove (free) an element from the hash table
int vfc_remove_probe(vfc_probes *probes, char *testName, char *varName) {

  if (probes == NULL) {
    return 1;
  }

  // Get the key, which is : testName + "," + varName
  char *key = gen_probe_key(testName, varName);

  vfc_hashmap_remove(probes->map, vfc_hashmap_str_function(key));

  return 0;
}

// Return the number of probes stored in the hashmap
unsigned int vfc_num_probes(vfc_probes *probes) {
  return vfc_hashmap_num_items(probes->map);
}

// Dump probes in a .csv file (the double values are converted to hex), then
// free it.
int vfc_dump_probes(vfc_probes *probes) {

  if (probes == NULL) {
    return 1;
  }

  // Get export path from the VFC_PROBES_OUTPUT env variable
  char *exportPath = getenv("VFC_PROBES_OUTPUT");
  if (!exportPath) {
    printf("Warning [verificarlo]: VFC_PROBES_OUTPUT is not set, probes will \
            not be dumped\n");
    vfc_free_probes(probes);
    return 0;
  }

  FILE *fp = fopen(exportPath, "w");

  if (fp == NULL) {
    fprintf(stderr,
            "Error [verificarlo]: impossible to open the CSV file to save your \
            probes (\"%s\")\n",
            exportPath);
    exit(1);
  }

  // First line gives the column names
  fprintf(fp, "test,variable,value\n");

  // Iterate over all table elements
  vfc_probe_node *probe = NULL;
  for (int i = 0; i < probes->map->capacity; i++) {
    probe = (vfc_probe_node *)get_value_at(probes->map->items, i);
    if (probe != NULL) {
      fprintf(fp, "%s,%a\n", probe->key, probe->value);
    }
  }

  fclose(fp);

  vfc_free_probes(probes);

  return 0;
}

#endif
