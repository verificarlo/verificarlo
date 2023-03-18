/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "interflop_stdlib.h"

// #include <stdlib.h>

#define HASH_MULTIPLIER 31
static const unsigned int hashmap_prime_1 = 73;
static const unsigned int hashmap_prime_2 = 5009;

#ifndef __VFC_HASHMAP_HEADER__

struct vfc_hashmap_st {
  ISize_t nbits;
  ISize_t mask;

  ISize_t capacity;
  ISize_t *items;
  ISize_t nitems;
  ISize_t n_deleted_items;
};
typedef struct vfc_hashmap_st *vfc_hashmap_t;

// allocate and initialize the map
vfc_hashmap_t vfc_hashmap_create();

// get the value at an index of a map
ISize_t get_value_at(ISize_t *items, ISize_t i);

// get the key at an index of a map
ISize_t get_key_at(ISize_t *items, ISize_t i);

// set the value at an index of a map
void set_value_at(ISize_t *items, ISize_t value, ISize_t i);

// set the key at an index of a map
void set_key_at(ISize_t *items, ISize_t key, ISize_t i);

// free the map
void vfc_hashmap_destroy(vfc_hashmap_t map);

// insert an element in the map
void vfc_hashmap_insert(vfc_hashmap_t map, ISize_t key, void *item);

// remove an element of the map
void vfc_hashmap_remove(vfc_hashmap_t map, ISize_t key);

// test if an element is in the map
char vfc_hashmap_have(vfc_hashmap_t map, ISize_t key);

// get an element of the map
void *vfc_hashmap_get(vfc_hashmap_t map, ISize_t key);

// get the number of elements in the map
ISize_t vfc_hashmap_num_items(vfc_hashmap_t map);

// Hash function
ISize_t vfc_hashmap_str_function(const char *id);

#endif

/***************** Verificarlo hashmap FUNCTIONS ********************
 * The following set of functions are used in backends and wrapper
 * to stock and access quickly internal data.
 *******************************************************************/

// free the map
void vfc_hashmap_destroy(vfc_hashmap_t map) {
  if (map) {
    interflop_free(map->items);
  }
  interflop_free(map);
}

// allocate and initialize the map
vfc_hashmap_t vfc_hashmap_create() {
  vfc_hashmap_t map =
      (vfc_hashmap_t)interflop_calloc(1, sizeof(struct vfc_hashmap_st));

  if (map == Null) {
    return Null;
  }
  map->nbits = 3;
  map->capacity = (ISize_t)(1 << map->nbits);
  map->mask = map->capacity - 1;
  // an item is now a value and a key
  map->items = (ISize_t *)interflop_calloc(map->capacity, 2 * sizeof(ISize_t));
  if (map->items == Null) {
    vfc_hashmap_destroy(map);
    return Null;
  }
  map->nitems = 0;
  map->n_deleted_items = 0;
  return map;
}

ISize_t get_value_at(ISize_t *items, ISize_t i) { return items[i * 2]; }

ISize_t get_key_at(ISize_t *items, ISize_t i) { return items[(i * 2) + 1]; }

void set_value_at(ISize_t *items, ISize_t value, ISize_t i) {
  items[i * 2] = value;
}

void set_key_at(ISize_t *items, ISize_t key, ISize_t i) {
  items[(i * 2) + 1] = key;
}

// add a member in the table
static int hashmap_add_member(vfc_hashmap_t map, ISize_t key, void *item) {
  ISize_t value = (ISize_t)item;
  ISize_t ii;

  if (value == 0 || value == 1) {
    return -1;
  }

  ii = map->mask & (hashmap_prime_1 * key);

  while (get_value_at(map->items, ii) != 0 &&
         get_value_at(map->items, ii) != 1) {
    if (get_value_at(map->items, ii) == value) {
      return 0;
    } else {
      /* search free slot */
      ii = map->mask & (ii + hashmap_prime_2);
    }
  }
  map->nitems++;
  if (get_value_at(map->items, ii) == 1) {
    map->n_deleted_items--;
  }

  set_value_at(map->items, value, ii);
  set_key_at(map->items, key, ii);

  return 1;
}

// rehash the table if necessary
static void maybe_rehash_map(vfc_hashmap_t map) {
  ISize_t *old_items;
  ISize_t old_capacity, ii;

  if (map->nitems + map->n_deleted_items >= (double)map->capacity * 0.85) {
    old_items = map->items;
    old_capacity = map->capacity;
    map->nbits++;
    map->capacity = (ISize_t)(1 << map->nbits);
    map->mask = map->capacity - 1;
    map->items =
        (ISize_t *)interflop_calloc(map->capacity, 2 * sizeof(ISize_t));
    map->nitems = 0;
    map->n_deleted_items = 0;
    for (ii = 0; ii < old_capacity; ii++) {
      hashmap_add_member(map, get_key_at(old_items, ii),
                         (void *)get_value_at(old_items, ii));
    }
    interflop_free(old_items);
  }
}

// insert an element in the map
void vfc_hashmap_insert(vfc_hashmap_t map, ISize_t key, void *item) {
  hashmap_add_member(map, key, item);
  maybe_rehash_map(map);
}

// remove an element of the map
void vfc_hashmap_remove(vfc_hashmap_t map, ISize_t key) {
  ISize_t ii = map->mask & (hashmap_prime_1 * key);

  while (get_value_at(map->items, ii) != 0) {
    if (get_key_at(map->items, ii) == key) {
      set_value_at(map->items, 1, ii);
      map->nitems--;
      map->n_deleted_items++;
      break;
    } else {
      ii = map->mask & (ii + hashmap_prime_2);
    }
  }
}

// test if an element is in the map
char vfc_hashmap_have(vfc_hashmap_t map, ISize_t key) {
  ISize_t ii = map->mask & (hashmap_prime_1 * key);

  while (get_value_at(map->items, ii) != 0) {
    if (get_key_at(map->items, ii) == key) {
      return 1;
    } else {
      ii = map->mask & (ii + hashmap_prime_2);
    }
  }
  return 0;
}

// get an element of the map
void *vfc_hashmap_get(vfc_hashmap_t map, ISize_t key) {
  ISize_t ii = map->mask & (hashmap_prime_1 * key);

  while (get_value_at(map->items, ii) != 0) {
    if (get_key_at(map->items, ii) == key) {
      return (void *)get_value_at(map->items, ii);
    } else {
      ii = map->mask & (ii + hashmap_prime_2);
    }
  }
  return Null;
}

// get the number of elements in the map
ISize_t vfc_hashmap_num_items(vfc_hashmap_t map) { return map->nitems; }

// Hash function for strings
ISize_t vfc_hashmap_str_function(const char *id) {
  unsigned const char *us;

  us = (unsigned const char *)id;

  ISize_t index = 0;

  while (*us != '\0') {
    index = index * HASH_MULTIPLIER + *us;
    us++;
  }

  return index;
}

// Free the hashmap
void vfc_hashmap_free(vfc_hashmap_t map) {
  for (ISize_t ii = 0; ii < map->capacity; ii++)
    if (get_value_at(map->items, ii) != 0 && get_value_at(map->items, ii) != 0)
      interflop_free((void *)get_value_at(map->items, ii));
}