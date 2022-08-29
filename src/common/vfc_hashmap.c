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

#include <stdlib.h>

#define HASH_MULTIPLIER 31
static const unsigned int hashmap_prime_1 = 73;
static const unsigned int hashmap_prime_2 = 5009;

#ifndef __VFC_HASHMAP_HEADER__

struct vfc_hashmap_st {
  size_t nbits;
  size_t mask;

  size_t capacity;
  size_t *items;
  size_t nitems;
  size_t n_deleted_items;
};
typedef struct vfc_hashmap_st *vfc_hashmap_t;

// allocate and initialize the map
vfc_hashmap_t vfc_hashmap_create();

// get the value at an index of a map
size_t get_value_at(size_t *items, size_t i);

// get the key at an index of a map
size_t get_key_at(size_t *items, size_t i);

// set the value at an index of a map
void set_value_at(size_t *items, size_t value, size_t i);

// set the key at an index of a map
void set_key_at(size_t *items, size_t key, size_t i);

// free the map
void vfc_hashmap_destroy(vfc_hashmap_t map);

// insert an element in the map
void vfc_hashmap_insert(vfc_hashmap_t map, size_t key, void *item);

// remove an element of the map
void vfc_hashmap_remove(vfc_hashmap_t map, size_t key);

// test if an element is in the map
char vfc_hashmap_have(vfc_hashmap_t map, size_t key);

// get an element of the map
void *vfc_hashmap_get(vfc_hashmap_t map, size_t key);

// get the number of elements in the map
size_t vfc_hashmap_num_items(vfc_hashmap_t map);

// Hash function
size_t vfc_hashmap_str_function(const char *id);

#endif

/***************** Verificarlo hashmap FUNCTIONS ********************
 * The following set of functions are used in backends and wrapper
 * to stock and access quickly internal data.
 *******************************************************************/

// free the map
void vfc_hashmap_destroy(vfc_hashmap_t map) {
  if (map) {
    free(map->items);
  }
  free(map);
}

// allocate and initialize the map
vfc_hashmap_t vfc_hashmap_create() {
  vfc_hashmap_t map = (vfc_hashmap_t)calloc(1, sizeof(struct vfc_hashmap_st));

  if (map == NULL) {
    return NULL;
  }
  map->nbits = 3;
  map->capacity = (size_t)(1 << map->nbits);
  map->mask = map->capacity - 1;
  // an item is now a value and a key
  map->items = (size_t *)calloc(map->capacity, 2 * sizeof(size_t));
  if (map->items == NULL) {
    vfc_hashmap_destroy(map);
    return NULL;
  }
  map->nitems = 0;
  map->n_deleted_items = 0;
  return map;
}

size_t get_value_at(size_t *items, size_t i) { return items[i * 2]; }

size_t get_key_at(size_t *items, size_t i) { return items[(i * 2) + 1]; }

void set_value_at(size_t *items, size_t value, size_t i) {
  items[i * 2] = value;
}

void set_key_at(size_t *items, size_t key, size_t i) {
  items[(i * 2) + 1] = key;
}

// add a member in the table
static int hashmap_add_member(vfc_hashmap_t map, size_t key, void *item) {
  size_t value = (size_t)item;
  size_t ii;

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
  size_t *old_items;
  size_t old_capacity, ii;

  if (map->nitems + map->n_deleted_items >= (double)map->capacity * 0.85) {
    old_items = map->items;
    old_capacity = map->capacity;
    map->nbits++;
    map->capacity = (size_t)(1 << map->nbits);
    map->mask = map->capacity - 1;
    map->items = (size_t *)calloc(map->capacity, 2 * sizeof(size_t));
    map->nitems = 0;
    map->n_deleted_items = 0;
    for (ii = 0; ii < old_capacity; ii++) {
      hashmap_add_member(map, get_key_at(old_items, ii),
                         (void *)get_value_at(old_items, ii));
    }
    free(old_items);
  }
}

// insert an element in the map
void vfc_hashmap_insert(vfc_hashmap_t map, size_t key, void *item) {
  hashmap_add_member(map, key, item);
  maybe_rehash_map(map);
}

// remove an element of the map
void vfc_hashmap_remove(vfc_hashmap_t map, size_t key) {
  size_t ii = map->mask & (hashmap_prime_1 * key);

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
char vfc_hashmap_have(vfc_hashmap_t map, size_t key) {
  size_t ii = map->mask & (hashmap_prime_1 * key);

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
void *vfc_hashmap_get(vfc_hashmap_t map, size_t key) {
  size_t ii = map->mask & (hashmap_prime_1 * key);

  while (get_value_at(map->items, ii) != 0) {
    if (get_key_at(map->items, ii) == key) {
      return (void *)get_value_at(map->items, ii);
    } else {
      ii = map->mask & (ii + hashmap_prime_2);
    }
  }
  return NULL;
}

// get the number of elements in the map
size_t vfc_hashmap_num_items(vfc_hashmap_t map) { return map->nitems; }

// Hash function for strings
size_t vfc_hashmap_str_function(const char *id) {
  unsigned const char *us;

  us = (unsigned const char *)id;

  size_t index = 0;

  while (*us != '\0') {
    index = index * HASH_MULTIPLIER + *us;
    us++;
  }

  return index;
}

// Free the hashmap
void vfc_hashmap_free(vfc_hashmap_t map) {
  for (size_t ii = 0; ii < map->capacity; ii++)
    if (get_value_at(map->items, ii) != 0 && get_value_at(map->items, ii) != 0)
      free((void *)get_value_at(map->items, ii));
}