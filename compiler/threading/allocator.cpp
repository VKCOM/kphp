// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory.h>

#include "common/sanitizer.h"
#include "common/container_of.h"
#include "common/wrappers/likely.h"

#include "compiler/threading/tls.h"

#if !ASAN_ENABLED && !defined(__clang__)
extern "C" {
extern decltype(malloc) __libc_malloc;
extern decltype(free) __libc_free;
extern decltype(calloc) __libc_calloc;
extern decltype(realloc) __libc_realloc;

struct block_t {
  union {
    size_t size;
    block_t *next;
  };
  char data[0];
};

#define MAX_BLOCK_SIZE 16384

class ZAllocatorRaw {
private:
  char *buf{nullptr};
  size_t left{0};
  block_t *free_blocks[MAX_BLOCK_SIZE >> 3]{nullptr};
  size_t memory_usage{0};
  size_t memory_total_allocated{0};

public:
  size_t get_memory_usage() const noexcept {
    return memory_usage;
  }

  size_t get_memory_total_allocated() const noexcept {
    return memory_total_allocated;
  }

  block_t *to_block(void *ptr) {
    return container_of(ptr, block_t, data);
  }

  void *from_block(block_t *block) {
    return &block->data;
  }

  block_t *block_alloc(size_t size) {
    size_t block_size = size + offsetof (block_t, data);
    block_t *res;
    if (size >= MAX_BLOCK_SIZE) {
      res = (block_t *)__libc_malloc(block_size);
    } else {
      size_t bucket_id = size >> 3;
      if (free_blocks[bucket_id] != nullptr) {
        res = free_blocks[bucket_id];
        free_blocks[bucket_id] = res->next;
      } else {
        if (unlikely (left < block_size)) {
          assert (malloc != __libc_malloc);
          buf = (char *)__libc_malloc(1 << 26);
          left = 1 << 26;
        }
        res = (block_t *)buf;
        buf += block_size;
        left -= block_size;
      }
    }
    res->size = size;
    memory_usage += res->size;
    memory_total_allocated += res->size;
    return res;
  }

  void block_free(block_t *block) {
    memory_usage -= block->size;
    if (block->size >= MAX_BLOCK_SIZE) {
      __libc_free(block);
    } else {
      size_t bucket_id = block->size >> 3;
      block->next = free_blocks[bucket_id];
      free_blocks[bucket_id] = block;
    }
  }

  inline void *alloc(size_t size) {
    size = (size + 7) & -8;

    block_t *block = block_alloc(size);
    return from_block(block);
  }

  inline void *realloc(void *ptr, size_t new_size) {
    if (ptr == nullptr) {
      return alloc(new_size);
    }
    if (new_size == 0) {
      free(ptr);
      return nullptr;
    }
    block_t *block = to_block(ptr);
    size_t old_size = block->size;
    void *res = alloc(new_size);
    memcpy(res, ptr, std::min(old_size, new_size));
    block_free(block);
    return res;
  }

  inline void *calloc(size_t size_a, size_t size_b) {
    size_t size = size_a * size_b;
    void *res = alloc(size);
    memset(res, 0, size);
    return res;
  }

  inline void free(void *ptr) {
    if (ptr == nullptr) {
      return;
    }
    block_t *block = to_block(ptr);
    block_free(block);
  }
};

static TLS<ZAllocatorRaw> zallocator;

void *malloc(size_t size) {
  assert (malloc != __libc_malloc);
  return zallocator->alloc(size);
}
void *calloc(size_t size_a, size_t size_b) {
  return zallocator->calloc(size_a, size_b);
}
void *realloc(void *ptr, size_t size) {
  return zallocator->realloc(ptr, size);
}
__attribute__((error("memalign function is assert(0), don't use it")))
void *memalign(size_t aligment __attribute__((unused)), size_t size __attribute__((unused))) {
  assert (0);
  return nullptr;
}
void free(void *ptr) {
  return zallocator->free(ptr);
}
}

size_t get_thread_memory_usage() {
  return zallocator->get_memory_usage();
}

size_t get_thread_memory_total_allocated() {
  return zallocator->get_memory_total_allocated();
}

#else

size_t get_thread_memory_usage() {
  return 0;
}

size_t get_thread_memory_total_allocated() {
  return 0;
}

#endif
