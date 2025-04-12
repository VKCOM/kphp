// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_ALLOCATORS_LOCKFREE_SLAB_H
#define KDB_COMMON_ALLOCATORS_LOCKFREE_SLAB_H

#include <sys/cdefs.h>
#include <sys/queue.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/allocators/freelist.h"
#include "common/cacheline.h"
#include "common/exact-division.h"

struct lockfree_slab_cache_tls;

struct lockfree_slab_block {
  struct lockfree_slab_cache_tls* cache_tls;
  LIST_ENTRY(lockfree_slab_block) blocks;
  uint64_t index;
  uint64_t empty_index;
  uint8_t payload[] __attribute__((aligned(__alignof__(void*))));
};
typedef struct lockfree_slab_block lockfree_slab_block_t;

struct lockfree_slab_cache;

struct lockfree_slab_cache_tls {
  struct lockfree_slab_cache* cache;
  LIST_HEAD(partial_head, lockfree_slab_block) partial;
  LIST_HEAD(empty_head, lockfree_slab_block) empty;
  LIST_HEAD(full_head, lockfree_slab_block) full;
  uint32_t partial_size;
  uint32_t empty_size;
  uint32_t full_size;
  uintptr_t extra;
  freelist_t waiting KDB_CACHELINE_ALIGNED;
} KDB_CACHELINE_ALIGNED;
typedef struct lockfree_slab_cache_tls lockfree_slab_cache_tls_t;

struct lockfree_slab_cache {
  uint32_t object_size;
  uint32_t aligned_object_size;
  uint32_t block_size;
  uint32_t objects_in_block;
  size_t block_alignment;
  uint64_t empty_index;
  exact_division_t exact_division;
  uintptr_t extra;
};
typedef struct lockfree_slab_cache lockfree_slab_cache_t;

static inline uint32_t lockfree_slab_cache_count_used_blocks(lockfree_slab_cache_tls_t* cache_tls) {
  return cache_tls->partial_size + cache_tls->empty_size + cache_tls->full_size;
}

void lockfree_slab_cache_init(lockfree_slab_cache_t* cache, uint32_t object_size);
void lockfree_slab_cache_register_thread(lockfree_slab_cache_t* cache, lockfree_slab_cache_tls_t* cache_tls);

void* lockfree_slab_cache_alloc(lockfree_slab_cache_tls_t* cache_tls);

static inline void* lockfree_slab_cache_alloc0(lockfree_slab_cache_tls_t* cache_tls) {
  void* object = lockfree_slab_cache_alloc(cache_tls);

  memset(object, '\0', cache_tls->cache->object_size);

  return object;
}

void lockfree_slab_cache_free(lockfree_slab_cache_tls_t* cache_tls, void* object);
void lockfree_slab_cache_clear(lockfree_slab_cache_tls_t* cache_tls);

#endif // KDB_COMMON_ALLOCATORS_LOCKFREE_SLAB_H
