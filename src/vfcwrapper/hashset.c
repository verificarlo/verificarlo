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

static const unsigned int prime_1 = 73;
static const unsigned int prime_2 = 5009;

hashset_t hashset_create() {
  hashset_t set = calloc(1, sizeof(struct hashset_st));

  if (set == NULL) {
    return NULL;
  }
  set->nbits = 3;
  set->capacity = (size_t)(1 << set->nbits);
  set->mask = set->capacity - 1;
  set->items = calloc(set->capacity, sizeof(size_t));
  if (set->items == NULL) {
    hashset_destroy(set);
    return NULL;
  }
  set->nitems = 0;
  set->n_deleted_items = 0;
  return set;
}

size_t hashset_num_items(hashset_t set) { return set->nitems; }

void hashset_destroy(hashset_t set) {
  if (set) {
    free(set->items);
  }
  free(set);
}

static int hashset_add_member(hashset_t set, void *item) {
  size_t value = (size_t)item;
  size_t ii;

  if (value == 0 || value == 1) {
    return -1;
  }

  ii = set->mask & (prime_1 * value);

  while (set->items[ii] != 0 && set->items[ii] != 1) {
    if (set->items[ii] == value) {
      return 0;
    } else {
      /* search free slot */
      ii = set->mask & (ii + prime_2);
    }
  }
  set->nitems++;
  if (set->items[ii] == 1) {
    set->n_deleted_items--;
  }
  set->items[ii] = value;
  return 1;
}

static void maybe_rehash(hashset_t set) {
  size_t *old_items;
  size_t old_capacity, ii;

  if (set->nitems + set->n_deleted_items >= (double)set->capacity * 0.85) {
    old_items = set->items;
    old_capacity = set->capacity;
    set->nbits++;
    set->capacity = (size_t)(1 << set->nbits);
    set->mask = set->capacity - 1;
    set->items = calloc(set->capacity, sizeof(size_t));
    set->nitems = 0;
    set->n_deleted_items = 0;
    assert(set->items);
    for (ii = 0; ii < old_capacity; ii++) {
      hashset_add_member(set, (void *)old_items[ii]);
    }
    free(old_items);
  }
}

int hashset_add(hashset_t set, void *item) {
  int rv = hashset_add_member(set, item);
  maybe_rehash(set);
  return rv;
}

int hashset_remove(hashset_t set, void *item) {
  size_t value = (size_t)item;
  size_t ii = set->mask & (prime_1 * value);

  while (set->items[ii] != 0) {
    if (set->items[ii] == value) {
      set->items[ii] = 1;
      set->nitems--;
      set->n_deleted_items++;
      return 1;
    } else {
      ii = set->mask & (ii + prime_2);
    }
  }
  return 0;
}

int hashset_is_member(hashset_t set, void *item) {
  size_t value = (size_t)item;
  size_t ii = set->mask & (prime_1 * value);

  while (set->items[ii] != 0) {
    if (set->items[ii] == value) {
      return 1;
    } else {
      ii = set->mask & (ii + prime_2);
    }
  }
  return 0;
}
