// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/allocators/lockfree-slab.h"

#include <assert.h>
#include <stdlib.h>

static inline uint32_t lockfree_slab_cache_object_id(lockfree_slab_cache_tls_t* cache_tls, lockfree_slab_block_t* block, const void* object) {
  const uintptr_t id = (uintptr_t)(object) - (uintptr_t)(block->payload);

  return exact_division(&cache_tls->cache->exact_division, id);
}

void lockfree_slab_cache_init(lockfree_slab_cache_t* cache, uint32_t object_size) {
  assert(object_size >= 8);

  enum { max_align = __alignof__(void*) };
  const uint32_t aligned_object_size = (object_size + max_align - 1) & ~(max_align - 1);

  cache->object_size = object_size;
  cache->aligned_object_size = aligned_object_size;
  exact_division_init(&cache->exact_division, aligned_object_size);

  const size_t unaligned_block_size = sizeof(lockfree_slab_block_t) + 32 * aligned_object_size;
  assert(unaligned_block_size < (1U << 31));
  cache->block_size = 1U << (32 - __builtin_clz(unaligned_block_size));
  cache->block_alignment = ~((size_t)(cache->block_size - 1));
  cache->objects_in_block = 32 + ((cache->block_size - unaligned_block_size) / cache->aligned_object_size);
  cache->empty_index = ~0ULL >> (64 - cache->objects_in_block);
}

void lockfree_slab_cache_register_thread(lockfree_slab_cache_t* cache, lockfree_slab_cache_tls_t* cache_tls) {
  cache_tls->cache = cache;
  LIST_INIT(&cache_tls->partial);
  LIST_INIT(&cache_tls->empty);
  LIST_INIT(&cache_tls->full);
  freelist_init(&cache_tls->waiting);
  cache_tls->partial_size = 0;
  cache_tls->empty_size = 0;
  cache_tls->full_size = 0;
}

static inline void* lockfree_slab_cache_alloc_internal(lockfree_slab_cache_tls_t* cache_tls) {
  if (!LIST_EMPTY(&cache_tls->partial)) {
    lockfree_slab_block_t* block = LIST_FIRST(&cache_tls->partial);

    const int id = __builtin_ctzll(block->index);
    block->index ^= 1ULL << id;

    if (!block->index) {
      LIST_REMOVE(block, blocks);
      LIST_INSERT_HEAD(&cache_tls->full, block, blocks);

      --cache_tls->partial_size;
      ++cache_tls->full_size;
    }

    return block->payload + id * cache_tls->cache->aligned_object_size;
  }

  if (!LIST_EMPTY(&cache_tls->empty)) {
    lockfree_slab_block_t* block = LIST_FIRST(&cache_tls->empty);
    LIST_REMOVE(block, blocks);
    LIST_INSERT_HEAD(&cache_tls->partial, block, blocks);

    block->index = cache_tls->cache->empty_index & (cache_tls->cache->empty_index - 1);

    --cache_tls->empty_size;
    ++cache_tls->partial_size;

    return block->payload;
  }

  return NULL;
}

static inline void lockfree_slab_cache_free_local(lockfree_slab_cache_tls_t* cache_tls, lockfree_slab_block_t* block, void* object) {
  const uint32_t id = lockfree_slab_cache_object_id(cache_tls, block, object);

  if (!block->index) {
    block->index = 1ULL << id;

    LIST_REMOVE(block, blocks);
    LIST_INSERT_HEAD(&cache_tls->partial, block, blocks);

    --cache_tls->full_size;
    ++cache_tls->partial_size;

    return;
  }

  block->index |= 1ULL << id;

  if (block->index == cache_tls->cache->empty_index) {
    LIST_REMOVE(block, blocks);

    if (cache_tls->empty_size > 4 && cache_tls->empty_size * 4 > (cache_tls->full_size + cache_tls->partial_size / 2)) {
      free(block);
    } else {
      LIST_INSERT_HEAD(&cache_tls->empty, block, blocks);
      ++cache_tls->empty_size;
    }

    --cache_tls->partial_size;
  }
}

void* lockfree_slab_cache_alloc(lockfree_slab_cache_tls_t* cache_tls) {
  void* object = lockfree_slab_cache_alloc_internal(cache_tls);

  if (!object) {
    object = freelist_get(&cache_tls->waiting);

    if (object) {
      lockfree_slab_block_t* block = (lockfree_slab_block_t*)((uintptr_t)(object)&cache_tls->cache->block_alignment);
      lockfree_slab_cache_free_local(cache_tls, block, object);

      object = lockfree_slab_cache_alloc_internal(cache_tls);
    }

    if (!object) {
      lockfree_slab_block_t* block;
      if (posix_memalign((void**)&block, cache_tls->cache->block_size, cache_tls->cache->block_size)) {
        return NULL;
      }

      block->cache_tls = cache_tls;
      block->index = cache_tls->cache->empty_index & (cache_tls->cache->empty_index - 1);

      LIST_INSERT_HEAD(&cache_tls->partial, block, blocks);
      ++cache_tls->partial_size;

      object = block->payload;
    }
  }

  return object;
}

void lockfree_slab_cache_free(lockfree_slab_cache_tls_t* cache_tls, void* object) {
  lockfree_slab_block_t* block = (lockfree_slab_block_t*)((uintptr_t)(object)&cache_tls->cache->block_alignment);

  if (block->cache_tls != cache_tls) {
    freelist_put(&block->cache_tls->waiting, object);
    return;
  }

  return lockfree_slab_cache_free_local(cache_tls, block, object);
}

void lockfree_slab_cache_clear(lockfree_slab_cache_tls_t* cache_tls) {
  while (!LIST_EMPTY(&cache_tls->full)) {
    lockfree_slab_block_t* block = LIST_FIRST(&cache_tls->full);
    LIST_REMOVE(block, blocks);
    free(block);
  }

  while (!LIST_EMPTY(&cache_tls->partial)) {
    lockfree_slab_block_t* block = LIST_FIRST(&cache_tls->partial);
    LIST_REMOVE(block, blocks);
    free(block);
  }

  while (!LIST_EMPTY(&cache_tls->empty)) {
    lockfree_slab_block_t* block = LIST_FIRST(&cache_tls->empty);
    LIST_REMOVE(block, blocks);
    free(block);
  }
}
