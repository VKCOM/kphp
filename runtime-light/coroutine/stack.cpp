#include "runtime-light/coroutine/stack.h"

#include "runtime-light/context.h"
#include "runtime-light/allocator/allocator.h"

#include <random>

CoroutineStack::CoroutineStack() : size(0) {
  printf("init coro stack\n");
  constexpr int64_t initial_capacity = 16;
  const Allocator & allocator = *get_platform_allocator();
  capacity = initial_capacity;
  start = static_cast<char *>(allocator.alloc(initial_capacity));
  php_assert(start != nullptr);
}

void *CoroutineStack::push(std::size_t n) {
//  return get_platform_allocator()->alloc(n);
  printf("push coro on stack %zu. Size %ld, capacity %ld\n", n, size, capacity);
  while (n > (capacity - size)) {
    resize(capacity * 5);
  }
  char * buff = start + size;
  size += n;
  return buff;
}

void CoroutineStack::pop(void * ptr, std::size_t n) {
//  return get_platform_allocator()->free(ptr);
  printf("pop coro on stack %zu\n", n);
  size -= n;
  php_assert((start + size) == ptr);
}

void CoroutineStack::resize(int64_t new_capacity) {
  printf("resize coro stack from %d to %d\n", capacity, new_capacity);
  const Allocator & allocator = *get_platform_allocator();
  char * new_buffer = static_cast<char *>(allocator.alloc(new_capacity));
  php_assert(new_buffer != nullptr);
  memcpy(new_buffer, start, size);
  allocator.free(start);
  start = new_buffer;
  capacity = new_capacity;
}

CoroutineStack::~CoroutineStack() {
  printf("delete coro stack\n");
  const Allocator & allocator = *get_platform_allocator();
  allocator.free(start);
}
