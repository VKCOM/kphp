// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/allocators/freelist.h"

#include <stdbool.h>

struct freelist_node {
  tagged_ptr_t next;
};
typedef struct freelist_node freelist_node_t;

void freelist_init(freelist_t* freelist) {
  tagged_ptr_pack(&freelist->pool, NULL, 0);
}

static tagged_ptr_t freelist_pool_top(freelist_t* freelist) {
  return tagged_ptr_from_uintptr(__sync_fetch_and_add((uintptr_t*)&freelist->pool, 0));
}

static bool freelist_pool_cas(freelist_t* freelist, uintptr_t old_value, uintptr_t new_value) {
  return tagged_ptr_cas(&freelist->pool, old_value, new_value);
}

void* freelist_get(freelist_t* freelist) {
  for (;;) {
    tagged_ptr_t old_pool = freelist_pool_top(freelist);
    freelist_node_t* old_pool_ptr = (freelist_node_t*)tagged_ptr_get_ptr(&old_pool);

    if (!old_pool_ptr) {
      return NULL;
    }

    freelist_node_t* new_pool_ptr = (freelist_node_t*)tagged_ptr_get_ptr(&old_pool_ptr->next);
    tagged_ptr_t new_pool;
    tagged_ptr_pack(&new_pool, new_pool_ptr, tagged_ptr_get_next_tag(&old_pool));

    if (freelist_pool_cas(freelist, tagged_ptr_to_uintptr(&old_pool), tagged_ptr_to_uintptr(&new_pool))) {
      return old_pool_ptr;
    }
  }
}

bool freelist_try_put(freelist_t* freelist, void* ptr) {
  freelist_node_t* node = (freelist_node_t*)ptr;

  tagged_ptr_t old_pool = freelist_pool_top(freelist);

  tagged_ptr_t new_pool;
  tagged_ptr_pack(&new_pool, node, tagged_ptr_get_tag(&old_pool));
  tagged_ptr_set_ptr(&node->next, tagged_ptr_get_ptr(&old_pool));

  return freelist_pool_cas(freelist, tagged_ptr_to_uintptr(&old_pool), tagged_ptr_to_uintptr(&new_pool));
}

void freelist_put(freelist_t* freelist, void* ptr) {
  while (!freelist_try_put(freelist, ptr)) {
  };
}
