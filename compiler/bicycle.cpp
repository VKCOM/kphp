#include "compiler/bicycle.h"

#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>

#include "common/container_of.h"

__thread int bicycle_thread_id;

/*** Malloc hooks ***/
#ifndef __SANITIZE_ADDRESS__
extern "C" {
extern decltype(malloc) __libc_malloc;
extern decltype(free) __libc_free;
extern decltype(calloc) __libc_calloc;
extern decltype(realloc) __libc_realloc;

typedef struct block_t_tmp block_t;
struct block_t_tmp {
  union {
    size_t size;
    block_t *next;
  };
  char data[0];
};

#define MAX_BLOCK_SIZE 16384

class ZAllocatorRaw {
private:
  char *buf;
  size_t left;
  block_t *free_blocks[MAX_BLOCK_SIZE >> 3];
public:
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
    return res;
  }

  void block_free(block_t *block) {
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
    memcpy(res, ptr, min(old_size, new_size));
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

TLS<ZAllocatorRaw> zallocator;

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
#endif

int get_thread_id() {
  return bicycle_thread_id;
}

void set_thread_id(int new_thread_id) {
  bicycle_thread_id = new_thread_id;
}

/*** ThreadLocalStorage ***/

volatile int bicycle_counter;

TLS<Profiler> profiler;


typedef char pstr_buff_t[5000];
TLS<pstr_buff_t> pstr_buff;

char *bicycle_dl_pstr(char const *msg, ...) {
  pstr_buff_t &s = *pstr_buff;
  va_list args;

  va_start (args, msg);
  vsnprintf(s, 5000, msg, args);
  va_end (args);

  return s;
}
