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

#ifndef __VFC_HASHMAP_H__
#define __VFC_HASHMAP_H__

#define __VFC_HASHMAP_HEADER__

#include "interflop/interflop_stdlib.h"

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
vfc_hashmap_t vfc_hashmap_create(void);

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

// Hash function for strings
ISize_t vfc_hashmap_str_function(const char *id);

// Free the hashmap
void vfc_hashmap_free(vfc_hashmap_t map);

#endif