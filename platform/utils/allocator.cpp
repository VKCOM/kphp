#include "platform/platform.h"

#include <cstdio>
#include <malloc.h>

#include <mutex>

std::mutex mutex;

void *platform::allocate(size_t n) noexcept {
  std::lock_guard<std::mutex> guard(mutex);
  void *ptr = malloc(n);
#ifdef DEBUG
  printf("Script allocate %d bytes on ptr %p\n", n, ptr);
#endif
  return ptr;
}

void *platform::allocate0(size_t n) noexcept {
  std::lock_guard<std::mutex> guard(mutex);
  return malloc(n);
}

void *platform::reallocate(void *p, size_t new_size, size_t old_size) noexcept {
  std::lock_guard<std::mutex> guard(mutex);
  return realloc(p, old_size);
}

void platform::deallocate(void *p, size_t n) noexcept {
  std::lock_guard<std::mutex> guard(mutex);
#ifdef DEBUG
  printf("Script free %d bytes on ptr %p\n", n, p);
#endif
  return free(p);
}