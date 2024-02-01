#include "platform.h"

#include <malloc.h>

void *platform::allocate(size_t n) noexcept {
  return malloc(n);
}

void *platform::allocate0(size_t n) noexcept {
  return malloc(n);
}

void *platform::reallocate(void *p, size_t new_size, size_t old_size) noexcept {
  return realloc(p, old_size);
}

void platform::deallocate(void *p, size_t n) noexcept {
  return free(p);
}